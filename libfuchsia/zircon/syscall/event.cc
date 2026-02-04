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
#include <object/event_dispatcher.h>
#include <object/handle.h>

zx_status_t zx_event_create(uint32_t options, zx_handle_t* out) {
  if (!out)
    return ZX_ERR_INVALID_ARGS;

  std::shared_ptr<EventDispatcher> event;
  zx_status_t status = EventDispatcher::Create(0u, &event, nullptr);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(event));
  return ZX_OK;
}

zx_status_t zx_event_create_fd(int fd, uint32_t options, zx_handle_t* out) {
  if (!out)
    return ZX_ERR_INVALID_ARGS;

  std::shared_ptr<EventDispatcher> event;
  zx_status_t status = EventDispatcher::Create(0u, &event, nullptr, fd);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(event));
  return ZX_OK;
}

zx_status_t zx_event_write(zx_handle_t handle) {
  std::shared_ptr<EventDispatcher> event;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_WRITE, &event);
  if (status != ZX_OK)
    return status;

  return event->Write();
}
