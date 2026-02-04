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

#include "object/dispatcher.h"

#include "lib/nebula/base/auto_lock.h"

Dispatcher::Dispatcher(zx_signals_t signals) : handle_count_(0u), signals_(signals) {}

Dispatcher::~Dispatcher() {}

void Dispatcher::AddObserver(StateObserver* observer, const StateObserver::CountInfo* cinfo) {
  AddObserverHelper(observer, cinfo, get_lock());
}

void Dispatcher::AddObserverHelper(StateObserver* observer, const StateObserver::CountInfo* cinfo,
                                   nebula::base::Mutex* mu) NO_THREAD_SAFETY_ANALYSIS {
  DCHECK(has_state_tracker());
  DCHECK(observer != nullptr);

  StateObserver::Flags flags;
  {
    nebula::base::AutoLock lock(mu);

    flags = observer->OnInitialize(signals_, cinfo);
    if (!(flags & StateObserver::kNeedRemoval))
      observers_.push_front(observer);
  }
  if (flags & StateObserver::kNeedRemoval)
    observer->OnRemoved();
}

void Dispatcher::RemoveObserver(StateObserver* observer) {
  DCHECK(has_state_tracker());

  nebula::base::AutoLock lock(get_lock());
  DCHECK(observer != nullptr);
  observers_.remove(observer);
}

zx_status_t Dispatcher::add_observer(StateObserver* observer) {
  if (!has_state_tracker())
    return ZX_ERR_NOT_SUPPORTED;
  AddObserver(observer, nullptr);
  return ZX_OK;
}

namespace {

template <typename Func>
StateObserver::Flags CancelWithFunc(Dispatcher::ObserverList* observers,
                                    nebula::base::Mutex* observer_lock, Func f) {
  StateObserver::Flags flags = 0;

  nebula::base::AutoLock lock(observer_lock);

  for (auto it = observers->begin(); it != observers->end();) {
    StateObserver::Flags it_flags = f(*it);
    flags |= it_flags;
    if (it_flags & StateObserver::kNeedRemoval) {
      auto to_remove = it;
      ++it;
      (*to_remove)->OnRemoved();
      observers->erase(to_remove);
    } else {
      ++it;
    }
  }

  // We've processed the removal flag, so strip it
  return flags & (~StateObserver::kNeedRemoval);
}

}  // namespace

bool Dispatcher::Cancel(Handle* handle) {
  StateObserver::Flags flags = CancelWithFunc(
      &observers_, get_lock(), [handle](StateObserver* obs) { return obs->OnCancel(handle); });

  // We could request a reschedule if kWokeThreads is asserted,
  // but cancellation is not likely to benefit from aggressive
  // rescheduling.
  return flags & StateObserver::kHandled;
}

bool Dispatcher::CancelByKey(Handle* handle, const void* port, uint64_t key) {
  StateObserver::Flags flags = CancelWithFunc(
      &observers_, get_lock(),
      [handle, port, key](StateObserver* obs) { return obs->OnCancelByKey(handle, port, key); });

  // We could request a reschedule if kWokeThreads is asserted,
  // but cancellation is not likely to benefit from aggressive
  // rescheduling.
  return flags & StateObserver::kHandled;
}

void Dispatcher::UpdateState(zx_signals_t clear_mask, zx_signals_t set_mask) {
  UpdateStateHelper(clear_mask, set_mask, get_lock());
}

void Dispatcher::UpdateStateLocked(zx_signals_t clear_mask, zx_signals_t set_mask) {
  DCHECK(has_state_tracker());

  nebula::base::NullLock mutex;
  UpdateStateHelper(clear_mask, set_mask, &mutex);
}

template <typename Mutex>
void Dispatcher::UpdateStateHelper(zx_signals_t clear_mask, zx_signals_t set_mask,
                                   Mutex* mu) NO_THREAD_SAFETY_ANALYSIS {
  nebula::base::AutoLock lock(mu);
  auto previous_signals = signals_;
  signals_ &= ~clear_mask;
  signals_ |= set_mask;

  if (previous_signals == signals_)
    return;

  UpdateInternalLocked(signals_);
}

StateObserver::Flags Dispatcher::UpdateInternalLocked(/*ObserverList* obs_to_remove,*/
                                                      zx_signals_t signals) {
  DCHECK(has_state_tracker());

  StateObserver::Flags flags = 0;

  for (auto it = observers_.begin(); it != observers_.end();) {
    StateObserver::Flags it_flags = (*it)->OnStateChange(signals);
    flags |= it_flags;
    if (it_flags & StateObserver::kNeedRemoval) {
      auto to_remove = it;
      ++it;
      (*to_remove)->OnRemoved();
      observers_.erase(to_remove);
    } else {
      ++it;
    }
  }

  // Filter out NeedRemoval flag because we processed that here
  return flags & (~StateObserver::kNeedRemoval);
}

zx_status_t SoloDispatcher::user_signal(uint32_t clear_mask, uint32_t set_mask, bool peer) {
  if (peer)
    return ZX_ERR_NOT_SUPPORTED;

  if (!has_state_tracker())
    return ZX_ERR_NOT_SUPPORTED;

  // Generic objects can set all USER_SIGNALs. Particular object
  // types (events and eventpairs) may be able to set more.
  auto allowed_signals = allowed_user_signals();
  if ((set_mask & ~allowed_signals) || (clear_mask & ~allowed_signals))
    return ZX_ERR_INVALID_ARGS;

  UpdateState(clear_mask, set_mask);
  return ZX_OK;
}