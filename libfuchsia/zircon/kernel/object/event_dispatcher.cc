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

#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <object/event_dispatcher.h>

#include "lib/nebula/base/logging.h"

zx_status_t EventDispatcher::Create(uint32_t options, std::shared_ptr<EventDispatcher>* dispatcher,
                                    zx_rights_t* rights) {
  std::shared_ptr<EventDispatcher> disp(new EventDispatcher(options));
  if (!disp)
    return ZX_ERR_NO_MEMORY;

  *dispatcher = disp;
  return ZX_OK;
}

zx_status_t EventDispatcher::Create(uint32_t options, std::shared_ptr<EventDispatcher>* dispatcher,
                                    zx_rights_t* rights, int fd) {
  std::shared_ptr<EventDispatcher> disp(new EventDispatcher(options));
  if (!disp)
    return ZX_ERR_NO_MEMORY;

  if (fd < 0) {
    return ZX_ERR_INVALID_ARGS;
  }

  disp->eventfd_.reset(fd);
  *dispatcher = disp;
  return ZX_OK;
}

EventDispatcher::EventDispatcher(uint32_t options) : signaled_(false) {
  CHECK_EQ(0u, options);
  eventfd_.reset(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
  DCHECK(eventfd_.ok());

  AddObserver(this, nullptr);
}

EventDispatcher::~EventDispatcher() {}

void EventDispatcher::SignalEventLocked() {
  if (!signaled_) {
    int err;
    do {
      err = eventfd_write(eventfd_, 1ull);
    } while (err == -1 && errno == EINTR);
    DCHECK_EQ(err, 0);
    signaled_ = true;
  }
}

void EventDispatcher::UnsignalEventLocked() {
  if (signaled_) {
    eventfd_t value;
    int err;
    do {
      err = eventfd_read(eventfd_, &value);
    } while (err < 0 && errno == EINTR);
    DCHECK_EQ(err, 0);
    signaled_ = false;
  }
}

void EventDispatcher::on_epoll_event(uint32_t events) {
  nebula::base::AutoLock lock(get_lock());

  uint32_t state = 0u;
  if (events & EPOLLIN) {
    signaled_ = true;
    state |= ZX_EVENT_SIGNALED;
  }

  UpdateStateLocked(0u, state);
}

StateObserver::Flags EventDispatcher::OnInitialize(zx_signals_t initial_state,
                                                   const CountInfo* cinfo) {
  return kHandled;
}

StateObserver::Flags EventDispatcher::OnStateChange(zx_signals_t new_state) {
  if (new_state & ZX_EVENT_SIGNALED)
    SignalEventLocked();
  else
    UnsignalEventLocked();

  return kHandled;
}

StateObserver::Flags EventDispatcher::OnCancel(const Handle* handle) { return kHandled; }

zx_status_t EventDispatcher::Write() {
  int err;
  do {
    err = eventfd_write(eventfd_, 1ull);
  } while (err == -1 && errno == EINTR);

  return (err == 0) ? ZX_OK : ZX_ERR_IO;
}