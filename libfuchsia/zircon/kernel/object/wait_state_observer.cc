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

#include <assert.h>

#include <object/dispatcher.h>
#include <object/handle.h>
#include <object/wait_state_observer.h>

#include "lib/nebula/base/logging.h"

WaitStateObserver::~WaitStateObserver() { DCHECK(!dispatcher_); }

zx_status_t WaitStateObserver::Begin(Event* event, Handle* handle, zx_signals_t watched_signals) {
  DCHECK(!dispatcher_);

  event_ = event;
  handle_ = handle;
  watched_signals_ = watched_signals;
  dispatcher_ = handle->dispatcher();
  wakeup_reasons_ = 0u;

  auto status = dispatcher_->add_observer(this);
  if (status != ZX_OK) {
    dispatcher_.reset();
    return status;
  }
  return ZX_OK;
}

zx_signals_t WaitStateObserver::End() {
  DCHECK(dispatcher_);
  DCHECK(dispatcher_->has_state_tracker());

  dispatcher_->RemoveObserver(this);
  dispatcher_.reset();

  // Return the set of reasons that we may have been woken.  Basically, this
  // is set of satisfied bits which were ever set while we were waiting on the list.
  return wakeup_reasons_;
}

StateObserver::Flags WaitStateObserver::OnInitialize(zx_signals_t initial_state,
                                                     const StateObserver::CountInfo* cinfo) {
  // Record the initial state of the state tracker as our wakeup reason.  If
  // we are going to become immediately signaled, the reason is contained
  // somewhere in this initial state.
  wakeup_reasons_ = initial_state;

  if (initial_state & watched_signals_) {
    event_->Signal();
  }

  return 0;
}

StateObserver::Flags WaitStateObserver::OnStateChange(zx_signals_t new_state) {
  // If we are still on our StateTracker's list of observers, and the
  // StateTracker's state has changed, accumulate the reasons that we may have
  // woken up.  In particular any satisfied bits which have become set
  // while we were on the list may have been reasons to wake up.
  wakeup_reasons_ |= new_state;

  if (new_state & watched_signals_) {
    event_->Signal();
  }

  return 0;
}

StateObserver::Flags WaitStateObserver::OnCancel(const Handle* handle) {
  if (handle == handle_) {
    wakeup_reasons_ |= ZX_SIGNAL_HANDLE_CLOSED;
    event_->Signal(ZX_ERR_CANCELED);
    return kHandled;
  } else {
    return 0;
  }
}
