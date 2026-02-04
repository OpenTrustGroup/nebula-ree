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

#include <time.h>
#include <zircon/syscalls.h>
#include <zircon/types.h>

#include <chrono>
#include <thread>

zx_time_t zx_clock_get(uint32_t clock_id) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec * ZX_SEC(1) + now.tv_nsec;
}

zx_status_t zx_nanosleep(zx_time_t deadline) {
  if (deadline == 0ull)
    return ZX_OK;

  zx_time_t now = zx_clock_get(ZX_CLOCK_MONOTONIC);
  if (deadline > now) {
    // TODO: Fix overflow issue.
    std::this_thread::sleep_for(std::chrono::nanoseconds(deadline - now));
  }
  return ZX_OK;
}

zx_time_t zx_deadline_after(zx_duration_t nanoseconds) {
  auto now = zx_clock_get(ZX_CLOCK_MONOTONIC);
  auto deadline = nanoseconds + now;
  // Check for overflow.  |nanoseconds| is unsigned, so we only get a
  // deadline in the past if overflow occurred.
  if (deadline < now)
    return ZX_TIME_INFINITE;
  return deadline;
}