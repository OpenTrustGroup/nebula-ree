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

#pragma once

#include <zircon/types.h>

#include <list>
#include <memory>
#include <mutex>

#include <object/epoll_handler.h>
#include <object/state_observer.h>

#include "lib/nebula/base/auto_lock.h"
#include "lib/nebula/base/logging.h"
#include "lib/nebula/base/mutex.h"
#include "lib/nebula/base/thread_annotations.h"

class Dispatcher : public std::enable_shared_from_this<Dispatcher>, public EpollHandler {
 public:
  explicit Dispatcher(zx_signals_t signals = 0u);
  virtual ~Dispatcher();

  // Must be called under the handle table lock.
  void increment_handle_count() { ++handle_count_; }

  // Must be called under the handle table lock.
  // Returns true exactly when the handle count goes to zero.
  bool decrement_handle_count() {
    --handle_count_;
    return handle_count_ == 0u;
  }

  // Must be called under the handle table lock.
  uint32_t current_handle_count() const { return handle_count_; }

  void AddObserver(StateObserver* observer, const StateObserver::CountInfo* cinfo);

  // Remove an observer (which must have been added).
  void RemoveObserver(StateObserver* observer);

  using ObserverList = std::list<StateObserver*>;

  bool Cancel(Handle* handle);
  bool CancelByKey(Handle* handle, const void* port, uint64_t key);

  std::shared_ptr<Dispatcher> self() { return shared_from_this(); }

  virtual zx_obj_type_t get_type() const = 0;

  virtual bool has_state_tracker() const { return false; }

  virtual zx_status_t user_signal(uint32_t clear_mask, uint32_t set_mask, bool peer) = 0;

  // This can be overriden by e.g. Events to allow for more signals.
  virtual zx_signals_t allowed_user_signals() const { return ZX_USER_SIGNAL_ALL; }

  virtual void on_zero_handles() {}

  virtual zx_status_t add_observer(StateObserver* observer);

  // Overrides EpollHandler
  void on_epoll_event(uint32_t events) override {}

 protected:
  void UpdateState(zx_signals_t clr_mask, zx_signals_t set_mask);
  void UpdateStateLocked(zx_signals_t clear_mask, zx_signals_t set_mask) REQUIRES(get_lock());

  zx_signals_t GetSignalsStateLocked() const REQUIRES(get_lock()) {
    DCHECK(has_state_tracker());
    return signals_;
  }
  virtual nebula::base::Mutex* get_lock() const = 0;

 private:
  void AddObserverHelper(StateObserver* observer, const StateObserver::CountInfo* cinfo,
                         nebula::base::Mutex* mutex);

  template <typename Mutex>
  void UpdateStateHelper(zx_signals_t clear_mask, zx_signals_t set_mask, Mutex* mutex);

  StateObserver::Flags UpdateInternalLocked(zx_signals_t signals) REQUIRES(get_lock());

  uint32_t handle_count_;
  ObserverList observers_ GUARDED_BY(get_lock());
  zx_signals_t signals_ GUARDED_BY(get_lock());
};

template <typename Endpoint>
class PeerHolder : public std::enable_shared_from_this<PeerHolder<Endpoint>> {
 public:
  PeerHolder() = default;
  ~PeerHolder() = default;

  nebula::base::Mutex* get_lock() const { return &lock_; }

  mutable nebula::base::Mutex lock_;
};

template <typename Self>
class PeeredDispatcher : public Dispatcher {
 public:
  // At construction, the object's state tracker is asserting
  // |signals|.
  explicit PeeredDispatcher(std::shared_ptr<PeerHolder<Self>> holder, zx_signals_t signals = 0u)
      : Dispatcher(signals), holder_(std::move(holder)) {}
  virtual ~PeeredDispatcher() = default;

  zx_status_t user_signal(uint32_t clear_mask, uint32_t set_mask,
                          bool peer) final NO_THREAD_SAFETY_ANALYSIS {
    auto allowed_signals = allowed_user_signals();
    if ((set_mask & ~allowed_signals) || (clear_mask & ~allowed_signals))
      return ZX_ERR_INVALID_ARGS;

    nebula::base::AutoLock locker(get_lock());

    if (!peer) {
      UpdateStateLocked(clear_mask, set_mask);
      return ZX_OK;
    }

    // object_signal() may race with handle_close() on another thread.
    if (!peer_)
      return ZX_ERR_PEER_CLOSED;
    peer_->UpdateStateLocked(clear_mask, set_mask);
    return ZX_OK;
  }

  nebula::base::Mutex* get_lock() const override { return holder_->get_lock(); }

 protected:
  zx_koid_t peer_koid_ = 0u;
  std::shared_ptr<Self> peer_ GUARDED_BY(get_lock());

 private:
  const std::shared_ptr<PeerHolder<Self>> holder_;
};

class SoloDispatcher : public Dispatcher {
 public:
  explicit SoloDispatcher(zx_signals_t signals = 0u) : Dispatcher(signals) {}

  zx_status_t user_signal(uint32_t clear_mask, uint32_t set_mask, bool peer) override;

 protected:
  nebula::base::Mutex* get_lock() const override { return &mu_; }
  mutable nebula::base::Mutex mu_;
};
