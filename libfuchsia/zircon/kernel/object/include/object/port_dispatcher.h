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

#include <semaphore.h>
#include <sys/epoll.h>
#include <zircon/syscalls/port.h>

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <object/common.h>
#include <object/dispatcher.h>
#include <object/epoll_handler.h>
#include <object/epoll_reader.h>

#include "lib/nebula/base/logging.h"
#include "lib/nebula/base/macros.h"
#include "lib/nebula/base/semaphore.h"
#include "lib/nebula/base/thread_annotations.h"
#include "lib/nebula/base/unique_fd.h"

class PortDispatcher;
class PortObserver;
struct PortPacket;

struct PortAllocator {
  virtual ~PortAllocator() = default;

  virtual PortPacket* Alloc() = 0;
  virtual void Free(PortPacket* port_packet) = 0;
};

struct PortPacket {
  PortPacket(const void* handle, PortAllocator* allocator);
  PortPacket(const PortPacket&) = delete;
  void operator=(PortPacket) = delete;
  ~PortPacket() = default;

  uint64_t key() const { return packet.key; }
  bool is_ephemeral() const { return allocator != nullptr; }
  bool& InContainer() { return is_InContainer; }

  zx_port_packet_t packet;
  const void* const handle;
  PortObserver* observer;
  PortAllocator* const allocator;
  bool is_InContainer;
};

class PortObserver : public StateObserver {
 public:
  PortObserver(uint32_t type, const Handle* handle, std::shared_ptr<PortDispatcher> port,
               uint64_t key, zx_signals_t signals);
  ~PortObserver();

  PortObserver(const PortObserver&) = delete;
  PortObserver& operator=(const PortObserver&) = delete;

  // StateObserver overrides.
  Flags OnInitialize(zx_signals_t initial_state, const StateObserver::CountInfo* cinfo) final;
  Flags OnStateChange(zx_signals_t new_state) final;
  Flags OnCancel(const Handle* handle) final;
  Flags OnCancelByKey(const Handle* handle, const void* port, uint64_t key) final;
  void OnRemoved() final;

  // The following method can only be called from
  // OnInitialize(), OnStateChange() and OnCancel().
  Flags MaybeQueue(zx_signals_t new_state, uint64_t count);

  const uint32_t type_;
  const zx_signals_t trigger_;
  PortPacket packet_;

  std::shared_ptr<PortDispatcher> const port_;
  std::shared_ptr<Dispatcher> target_;
};

class PortDispatcher final : public SoloDispatcher {
 public:
  static zx_status_t Create(uint32_t options, std::shared_ptr<PortDispatcher>* dispatcher,
                            zx_rights_t* rights);

  ~PortDispatcher() final;

  zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_PORT; }
  void on_zero_handles() final;

  zx_status_t Queue(PortPacket* port_packet, zx_signals_t observed, uint64_t count);
  zx_status_t QueueUser(const zx_port_packet_t& packet);
  zx_status_t Dequeue(zx_time_t deadline, zx_port_packet_t* packet);

  // Decides who is going to destroy the observer. If it returns |true| it
  // is the duty of the caller. If it is false it is the duty of the port.
  bool CanReap(PortObserver* observer, PortPacket* port_packet);

  // Called under the handle table lock.
  zx_status_t MakeObserver(uint32_t options, Handle* handle, uint64_t key, zx_signals_t signals);

  // Called under the handle table lock. Returns true if at least one packet was
  // removed from the queue.
  bool CancelQueued(const void* handle, uint64_t key);

  int get_fd() const final { return epoll_reader_.get_fd(); }

  void MaybeWatch(std::shared_ptr<Dispatcher> target) {
    if (target->get_fd() >= 0)
      epoll_reader_.Watch(target->get_fd(), target.get());
  }

  void MaybeUnwatch(std::shared_ptr<Dispatcher> target) {
    if (target->get_fd() >= 0)
      epoll_reader_.Unwatch(target->get_fd());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PortDispatcher);

  explicit PortDispatcher();

  bool BecomeActivePoller();
  void ResignActivePoller();

  EpollReader epoll_reader_;

  bool zero_handles_ GUARDED_BY(get_lock());
  std::list<PortPacket*> packets_ GUARDED_BY(get_lock());

  // A dummy identifier used to distinguish between Queue() and QueueUser().
  // See PortPacket::is_ephemeral().
  PortAllocator* kDummyPortAllocator =
      reinterpret_cast<PortAllocator*>(0x434C4C4154524F50);  // 'PORTALLC'

  static constexpr int kMaxEpollEvents = 128;
  epoll_event events_[kMaxEpollEvents];
  int num_events_;

  nebula::base::Semaphore semaphore_;
  std::atomic<intptr_t> active_poller_;
};