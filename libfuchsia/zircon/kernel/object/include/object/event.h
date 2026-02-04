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

#include <stdint.h>
#include <zircon/types.h>

#include <mutex>

#include "lib/nebula/base/macros.h"
#include "lib/nebula/base/unique_fd.h"
#include "lib/nebula/base/mutex.h"

class Event {
 public:
  Event(uint32_t flags = 0);

  zx_status_t Wait(zx_time_t deadline);
  void Signal(zx_status_t status = ZX_OK);
  zx_status_t Unsignal();

 private:
  DISALLOW_COPY_AND_ASSIGN(Event);

  void SignalLocked(zx_status_t status) REQUIRES(mu_);
  zx_status_t UnsignalLocked() REQUIRES(mu_);

  zx_status_t do_eventfd_poll(int timeout_ms);
  zx_status_t do_eventfd_read(zx_status_t* status);
  zx_status_t do_eventfd_write(zx_status_t status);

  uint32_t flags_;
  nebula::base::Mutex mu_;
  bool signaled_ GUARDED_BY(mu_);
  nebula::base::unique_fd eventfd_;
};