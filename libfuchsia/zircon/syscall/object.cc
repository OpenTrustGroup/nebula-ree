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

#include <zircon/syscalls.h>
#include <zircon/types.h>

#include <memory>

#include <object/common.h>
#include <object/event.h>
#include <object/port_dispatcher.h>
#include <object/wait_state_observer.h>

#include "lib/nebula/base/auto_lock.h"

zx_status_t zx_object_signal(zx_handle_t handle, uint32_t clear_mask, uint32_t set_mask) {
  std::shared_ptr<Dispatcher> dispatcher;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_SIGNAL, &dispatcher);
  if (status != ZX_OK)
    return status;

  return dispatcher->user_signal(clear_mask, set_mask, false);
}

zx_status_t zx_object_wait_one(zx_handle_t handle_value, zx_signals_t signals, zx_time_t deadline,
                               zx_signals_t* observed) {
  Event event;
  zx_status_t result;
  WaitStateObserver wait_state_observer;

  {
    nebula::base::AutoLock lock(GetHandleTableLock());

    Handle* handle = Handle::GetHandleLocked(handle_value);
    if (!handle)
      return ZX_ERR_BAD_HANDLE;

    result = wait_state_observer.Begin(&event, handle, signals);
    if (result != ZX_OK)
      return result;
  }

  // event_wait() will return ZX_OK if already signaled,
  // even if the deadline has passed.  It will return ZX_ERR_TIMED_OUT
  // after the deadline passes if the event has not been
  // signaled.
  result = event.Wait(deadline);

  // Regardless of wait outcome, we must call End().
  auto signals_state = wait_state_observer.End();

  if (observed) {
    *observed = signals_state;
  }

  if (signals_state & ZX_SIGNAL_HANDLE_CLOSED)
    return ZX_ERR_CANCELED;

  return result;
}

zx_status_t zx_object_wait_many(zx_wait_item_t* user_items, size_t count, zx_time_t deadline) {
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zx_object_wait_async(zx_handle_t handle_value, zx_handle_t port_handle, uint64_t key,
                                 zx_signals_t signals, uint32_t options) {
  if (port_handle == ZX_HANDLE_INVALID)
    return ZX_ERR_INVALID_ARGS;

  std::shared_ptr<PortDispatcher> port;
  zx_status_t status = GetDispatcherWithRights(port_handle, ZX_RIGHT_WRITE, &port);
  if (status != ZX_OK)
    return status;

  {
    nebula::base::AutoLock lock(GetHandleTableLock());
    Handle* handle = Handle::GetHandleLocked(handle_value);
    if (!handle)
      return ZX_ERR_BAD_HANDLE;

    return port->MakeObserver(options, handle, key, signals);
  }
}

int zx_object_get_fd(zx_handle_t handle) {
  std::shared_ptr<Dispatcher> disp;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_WRITE, &disp);
  if (status != ZX_OK) {
    LOG(ERROR) << "Failed to get fd: " << status;
    return -1;
  }

  return disp->get_fd();
}