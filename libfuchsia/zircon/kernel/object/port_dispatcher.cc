// Copyright (c) 2021 GoldenRiver Technology Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <object/handle.h>
#include <object/port_dispatcher.h>

#include "lib/nebula/base/auto_lock.h"
#include "lib/nebula/base/scopeguard.h"
#include "lib/nebula/base/time.h"

PortPacket::PortPacket(const void* handle, PortAllocator* allocator)
    : packet{}, handle(handle), observer(nullptr), allocator(allocator), is_InContainer(false) {
  // Note that packet is initialized to zeros.
  if (handle) {
    // Currently |handle| is only valid if the packets are not ephemeral
    // which means that PortObserver always uses the kernel heap.
    DCHECK(allocator == nullptr);
  }
}

PortObserver::PortObserver(uint32_t type, const Handle* handle,
                           std::shared_ptr<PortDispatcher> port, uint64_t key, zx_signals_t signals)
    : type_(type), trigger_(signals), packet_(handle, nullptr), port_(port) {
  DCHECK(handle != nullptr);

  auto& packet = packet_.packet;
  packet.status = ZX_OK;
  packet.key = key;
  packet.type = type_;
  packet.signal.trigger = trigger_;

  target_ = handle->dispatcher();
  DCHECK(target_ != nullptr);

  port_->MaybeWatch(target_);
}

PortObserver::~PortObserver() { port_->MaybeUnwatch(target_); }

StateObserver::Flags PortObserver::OnInitialize(zx_signals_t initial_state,
                                                const StateObserver::CountInfo* cinfo) {
  uint64_t count = 1u;

  if (cinfo) {
    for (const auto& entry : cinfo->entry) {
      if ((entry.signal & trigger_) && (entry.count > 0u)) {
        count = entry.count;
        break;
      }
    }
  }

  return MaybeQueue(initial_state, count);
}

StateObserver::Flags PortObserver::OnStateChange(zx_signals_t new_state) {
  return MaybeQueue(new_state, 1u);
}

StateObserver::Flags PortObserver::OnCancel(const Handle* handle) {
  if (packet_.handle == handle) {
    return kHandled | kNeedRemoval;
  } else {
    return 0;
  }
}

StateObserver::Flags PortObserver::OnCancelByKey(const Handle* handle, const void* port,
                                                 uint64_t key) {
  if ((packet_.handle != handle) || (packet_.key() != key) || (port_.get() != port))
    return 0;

  return kHandled | kNeedRemoval;
}

void PortObserver::OnRemoved() {
  if (port_->CanReap(this, &packet_)) {
    delete this;
  }
}

StateObserver::Flags PortObserver::MaybeQueue(zx_signals_t new_state, uint64_t count) {
  // Always called with the object state lock being held.
  if ((trigger_ & new_state) == 0u)
    return 0;

  auto status = port_->Queue(&packet_, new_state, count);

  if ((type_ == ZX_PKT_TYPE_SIGNAL_ONE) || (status < 0))
    return kNeedRemoval;

  return 0;
}

///////////////////////////////////////////////////////////////////////////////

PortDispatcher::PortDispatcher()
    : zero_handles_(false), num_events_(0), semaphore_(0), active_poller_(0) {}

PortDispatcher::~PortDispatcher() { DCHECK(zero_handles_); }

void PortDispatcher::on_zero_handles() {
  {
    nebula::base::AutoLock lock(get_lock());
    zero_handles_ = true;
  }
  while (Dequeue(0ull, nullptr) == ZX_OK) {
    // Cleanup packets and observers
  }
}

// static
zx_status_t PortDispatcher::Create(uint32_t options, std::shared_ptr<PortDispatcher>* dispatcher,
                                   zx_rights_t* rights) {
  std::shared_ptr<PortDispatcher> disp(new PortDispatcher);
  if (!disp)
    return ZX_ERR_NO_MEMORY;

  *dispatcher = disp;
  return ZX_OK;
}

zx_status_t PortDispatcher::Queue(PortPacket* port_packet, zx_signals_t observed, uint64_t count) {
  nebula::base::AutoLock lock(get_lock());

  if (zero_handles_)
    return ZX_ERR_BAD_STATE;

  if (observed) {
    if (port_packet->InContainer()) {
      port_packet->packet.signal.observed |= observed;
      // |count| is deliberately left as is.
      return ZX_OK;
    }
    port_packet->packet.signal.observed = observed;
    port_packet->packet.signal.count = count;
  }

  packets_.push_back(port_packet);
  port_packet->InContainer() = true;

  // TODO: Use EPOLLEXCLUSIVE(Linux 4.5) instead of wakeup_eventfd and semaphore.
  semaphore_.Post();
  epoll_reader_.Wakeup();

  return ZX_OK;
}

zx_status_t PortDispatcher::QueueUser(const zx_port_packet_t& packet) {
  auto port_packet = new PortPacket(nullptr, kDummyPortAllocator);
  if (!port_packet)
    return ZX_ERR_NO_MEMORY;

  port_packet->packet = packet;
  port_packet->packet.type = ZX_PKT_TYPE_USER;

  auto status = Queue(port_packet, 0u, 0u);
  if (status < 0)
    delete port_packet;
  return status;
}

bool PortDispatcher::BecomeActivePoller() {
  intptr_t expected = 0;
  bool success = active_poller_.compare_exchange_strong(expected, 1, std::memory_order_acq_rel,
                                                        std::memory_order_acquire);
  return success;
}

void PortDispatcher::ResignActivePoller() { active_poller_.store(0); }

zx_status_t PortDispatcher::Dequeue(zx_time_t deadline, zx_port_packet_t* out_packet) {
  while (true) {
    {
      nebula::base::AutoLock lock(get_lock());

      if (packets_.empty())
        goto wait;

      PortPacket* port_packet = packets_.front();
      packets_.pop_front();
      port_packet->InContainer() = false;

      if (out_packet != nullptr)
        *out_packet = port_packet->packet;

      PortObserver* observer = port_packet->observer;

      if (observer) {
        // Deleting the observer under the lock is fine because
        // the reference that holds to this PortDispatcher is by
        // construction not the last one. We need to do this under
        // the lock because another thread can call CanReap().
        delete observer;
      } else if (port_packet->is_ephemeral()) {
        delete port_packet;
      }
    }

    return ZX_OK;

  wait:
    // Ensure |deadline| is within range 0 <= |deadline| <= int64_max.
    deadline = std::min<zx_time_t>(std::chrono::nanoseconds::max().count(), deadline);

    zx_time_t now = current_time();
    int64_t timeout_ns = deadline > now ? deadline - now : 0;

    // There could be multiple threads enter Dequeue(), only one of them will become
    // |active_poller_| to perform epoll_wait(), others will wait on |semaphore_|.
    if (BecomeActivePoller()) {
      auto resign = nebula::base::make_scope_guard([this] { ResignActivePoller(); });

      int64_t timeout_ms = timeout_ns / nebula::base::kNanosPerMillisecond;

      zx_status_t status = epoll_reader_.WaitAndDispatch(timeout_ms);
      if (status != ZX_OK)
        return status;

      // Try to decrement the semaphore value by one. Balanced with |Queue|.
      semaphore_.TryWait();
    } else {
      bool waited = semaphore_.WaitWithTimeout(std::chrono::nanoseconds(timeout_ns));
      if (!waited)
        return ZX_ERR_TIMED_OUT;
    }
  }
}

bool PortDispatcher::CanReap(PortObserver* observer, PortPacket* port_packet) {
  nebula::base::AutoLock lock(get_lock());
  if (!port_packet->InContainer()) {
    return true;
  }
  // The destruction will happen when the packet is dequeued or in
  // CancelQueued()
  DCHECK(port_packet->observer == nullptr);
  port_packet->observer = observer;
  return false;
}

zx_status_t PortDispatcher::MakeObserver(uint32_t options, Handle* handle, uint64_t key,
                                         zx_signals_t signals) {
  // Called under the handle table lock.
  auto dispatcher = handle->dispatcher();
  if (!dispatcher->has_state_tracker())
    return ZX_ERR_NOT_SUPPORTED;

  uint32_t type;
  switch (options) {
    case ZX_WAIT_ASYNC_ONCE:
      type = ZX_PKT_TYPE_SIGNAL_ONE;
      break;
    case ZX_WAIT_ASYNC_REPEATING:
      type = ZX_PKT_TYPE_SIGNAL_REP;
      break;
    default:
      return ZX_ERR_INVALID_ARGS;
  }

  auto observer = new PortObserver(type, handle, std::static_pointer_cast<PortDispatcher>(self()),
                                   key, signals);

  dispatcher->add_observer(observer);
  return ZX_OK;
}

bool PortDispatcher::CancelQueued(const void* handle, uint64_t key) {
  nebula::base::AutoLock lock(get_lock());

  // This loop can take a while if there are many items.
  // In practice, the number of pending signal packets is
  // approximately the number of signaled _and_ watched
  // objects plus the number of pending user-queued
  // packets.
  //
  // There are two strategies to deal with too much
  // looping here if that is seen in practice.
  //
  // 1. Swap the |packets_| list for an empty list and
  //    release the lock. New arriving packets are
  //    added to the empty list while the loop happens.
  //    Readers will be blocked but the watched objects
  //    will be fully operational. Once processing
  //    is done the lists are appended.
  //
  // 2. Segregate user packets from signal packets
  //    and deliver them in order via timestamps or
  //    a side structure.

  bool packet_removed = false;

  for (auto it = packets_.begin(); it != packets_.end();) {
    if ((*it)->handle == nullptr) {
      ++it;
      continue;
    }

    if (((*it)->handle == handle) && ((*it)->key() == key)) {
      auto to_remove = it++;
      // Sanity check: races between |Queue| and |onRemove|.
      DCHECK((*to_remove)->observer);
      delete (*to_remove)->observer;
      packets_.erase(to_remove);
      packet_removed = true;
    } else {
      ++it;
    }
  }

  return packet_removed;
}