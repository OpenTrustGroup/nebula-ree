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

#include <cstring>

#include <object/common.h>
#include <object/handle.h>
#include <object/port_dispatcher.h>

#include "lib/nebula/base/auto_lock.h"

zx_status_t zx_port_create(uint32_t options, zx_handle_t* out) {
  std::shared_ptr<PortDispatcher> port;
  zx_status_t status = PortDispatcher::Create(0u, &port, nullptr);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(port));

  return ZX_OK;
}

zx_status_t zx_port_queue(zx_handle_t handle, const zx_port_packet_t* packet, size_t count) {
  if (count != 0u && count != 1u)
    return ZX_ERR_INVALID_ARGS;

  if (!packet)
    return ZX_ERR_INVALID_ARGS;

  std::shared_ptr<PortDispatcher> port;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_WRITE, &port);
  if (status != ZX_OK)
    return status;

  return port->QueueUser(*packet);
}

zx_status_t zx_port_wait(zx_handle_t handle, zx_time_t deadline, zx_port_packet_t* out,
                         size_t count) {
  if (count != 0u && count != 1u)
    return ZX_ERR_INVALID_ARGS;

  if (!out)
    return ZX_ERR_INVALID_ARGS;

  std::shared_ptr<PortDispatcher> port;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_READ, &port);
  if (status != ZX_OK)
    return status;

  zx_port_packet_t packet;
  status = port->Dequeue(deadline, &packet);
  if (status != ZX_OK)
    return status;

  memcpy(out, &packet, sizeof(packet));
  return ZX_OK;
}

zx_status_t zx_port_cancel(zx_handle_t handle, zx_handle_t source, uint64_t key) {
  std::shared_ptr<PortDispatcher> port;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_WRITE, &port);
  if (status != ZX_OK)
    return status;

  {
    nebula::base::AutoLock lock(GetHandleTableLock());
    Handle* watched = Handle::GetHandleLocked(source);
    if (!watched)
      return ZX_ERR_BAD_HANDLE;

    auto dispatcher = watched->dispatcher();
    if (!dispatcher->has_state_tracker())
      return ZX_ERR_NOT_SUPPORTED;

    bool had_observer = dispatcher->CancelByKey(watched, port.get(), key);
    bool packet_removed = port->CancelQueued(watched, key);
    return (had_observer || packet_removed) ? ZX_OK : ZX_ERR_NOT_FOUND;
  }
}
