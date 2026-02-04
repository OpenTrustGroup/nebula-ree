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
#include <object/handle.h>
#include <object/timer_dispatcher.h>

zx_status_t zx_timer_create(uint32_t options, uint32_t clock_id, zx_handle_t* out) {
  if (!out)
    return ZX_ERR_INVALID_ARGS;

  std::shared_ptr<TimerDispatcher> timer;
  zx_status_t status = TimerDispatcher::Create(options, &timer, nullptr);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(timer));
  return ZX_OK;
}

zx_status_t zx_timer_set(zx_handle_t handle, zx_time_t deadline, zx_duration_t slack) {
  std::shared_ptr<TimerDispatcher> timer;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_WRITE, &timer);
  if (status != ZX_OK)
    return status;
  return timer->Set(deadline, slack);
}