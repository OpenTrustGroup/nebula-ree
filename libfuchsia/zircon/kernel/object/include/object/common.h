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

#include <memory>

#include <object/dispatcher.h>
#include <object/handle.h>

#include "lib/nebula/base/logging.h"
#include "lib/nebula/base/auto_lock.h"

// See ProcessDispatcher |handle_table_lock_|.
std::mutex* GetHandleTableLock();

template <typename T>
static inline zx_status_t GetDispatcherWithRights(zx_handle_t handle_value,
                                                  zx_rights_t desired_rights,
                                                  std::shared_ptr<T>* dispatcher) {
  nebula::base::AutoLock lock(GetHandleTableLock());
  Handle* handle = Handle::GetHandleLocked(handle_value);
  if (!handle)
    return ZX_ERR_BAD_HANDLE;

  *dispatcher = std::static_pointer_cast<T>(handle->dispatcher());
  return ZX_OK;
}

static inline zx_time_t current_time() {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec * ZX_SEC(1) + now.tv_nsec;
}