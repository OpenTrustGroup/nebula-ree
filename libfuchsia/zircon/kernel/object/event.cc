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

#include <string.h>
#include <sys/eventfd.h>
#include <sys/poll.h>

#include <chrono>

#include <object/common.h>
#include <object/event.h>

#include "lib/nebula/base/logging.h"
#include "lib/nebula/base/auto_lock.h"

namespace {
// Writes zero (such as ZX_OK) to eventfd will not trigger any events,
// so we need some translation between counter value and zx_status_t.
eventfd_t __status_to_event(zx_status_t status) { return -(status) + 1; }
zx_status_t __event_to_status(eventfd_t event) { return -(event - 1); }
}  // namespace

Event::Event(uint32_t flags) : flags_(flags), signaled_(false) {
  CHECK_EQ(0u, flags);
  eventfd_.reset(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
  DCHECK(eventfd_.ok());
}

zx_status_t Event::Wait(zx_time_t deadline) {
  {
    nebula::base::AutoLock lock(&mu_);
    if (signaled_)
      return UnsignalLocked();
  }

  zx_time_t now = current_time();
  zx_time_t timeout_ns = deadline > now ? deadline - now : 0u;

  int timeout_ms = (timeout_ns / 1000) > (zx_time_t)std::numeric_limits<uint32_t>::max()
                       ? -1
                       : timeout_ns / ZX_MSEC(1);

  zx_status_t status = do_eventfd_poll(timeout_ms);
  if (status != ZX_OK)
    return status;

  return Unsignal();
}

void Event::Signal(zx_status_t status) {
  nebula::base::AutoLock lock(&mu_);
  SignalLocked(status);
}

void Event::SignalLocked(zx_status_t status) {
  if (!signaled_) {
    zx_status_t result = do_eventfd_write(status);
    DCHECK_EQ(result, ZX_OK);
    signaled_ = true;
  }
}

zx_status_t Event::Unsignal() {
  nebula::base::AutoLock lock(&mu_);
  return UnsignalLocked();
}

zx_status_t Event::UnsignalLocked() {
  zx_status_t result = ZX_OK;
  if (signaled_) {
    zx_status_t status = do_eventfd_read(&result);
    DCHECK_EQ(status, ZX_OK);
    signaled_ = false;
  }
  return result;
}

zx_status_t Event::do_eventfd_poll(int timeout_ms) {
  pollfd pfd;
  pfd.fd = eventfd_.get();
  pfd.events = POLLIN;
  int err;
  do {
    err = poll(&pfd, 1, timeout_ms);
  } while (err < 0 && errno == EINTR);
  if (err == 0)
    return ZX_ERR_TIMED_OUT;
  return ZX_OK;
}

zx_status_t Event::do_eventfd_read(zx_status_t* status) {
  eventfd_t value;
  int err;
  do {
    err = eventfd_read(eventfd_.get(), &value);
  } while (err < 0 && errno == EINTR);
  if (err < 0 && errno != EAGAIN)
    return ZX_ERR_IO;
  if (status)
    *status = __event_to_status(value);
  return ZX_OK;
}

zx_status_t Event::do_eventfd_write(zx_status_t status) {
  int err;
  do {
    err = eventfd_write(eventfd_.get(), __status_to_event(status));
  } while (err < 0 && errno == EINTR);
  if (err < 0)
    return ZX_ERR_IO;
  return ZX_OK;
}
