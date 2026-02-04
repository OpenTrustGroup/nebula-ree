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

#include <object/epoll_reader.h>

#include "lib/nebula/base/auto_lock.h"
#include "lib/nebula/base/logging.h"

EpollReader::EpollReader() {
  epoll_fd_.reset(epoll_create1(EPOLL_CLOEXEC));
  CHECK(epoll_fd_.ok()) << strerror(errno);

  wakeup_fd_.reset(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE));
  CHECK(wakeup_fd_.ok()) << strerror(errno);

  epoll_event event;
  event.events = EPOLLIN;  // EPOLLLT for EFD_SEMAPHORE
  event.data.ptr = this;

  CHECK_EQ(epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, wakeup_fd_.get(), &event), 0)
      << strerror(errno);
}

bool EpollReader::increment_watcher_count_locked(int fd) {
  return ++watcher_count_[fd] == 1;
}

bool EpollReader::decrement_watcher_count_locked(int fd) {
  return --watcher_count_[fd] == 0;
}

void EpollReader::Watch(int fd, EpollHandler* handler, uint32_t events) {
  CHECK_GE(fd, 0);
  CHECK(handler);

  nebula::base::AutoLock lock(&mu_);

  bool first_watcher = increment_watcher_count_locked(fd);
  if (first_watcher) {
    epoll_event event;
    event.events = events;
    event.data.ptr = handler;
    CHECK_EQ(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event), 0) << strerror(errno);
  }
}

void EpollReader::Unwatch(int fd) {
  CHECK_GE(fd, 0);

  nebula::base::AutoLock lock(&mu_);

  bool last_watcher = decrement_watcher_count_locked(fd);
  if (last_watcher) {
    epoll_event dummy;
    CHECK_EQ(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &dummy), 0) << strerror(errno);
  }
}

zx_status_t EpollReader::WaitAndDispatch(int timeout_ms) {
  epoll_event ev;
  int result;
  do {
    result = epoll_wait(epoll_fd_.get(), &ev, 1, timeout_ms);
  } while (result < 0 && errno == EINTR);
  if (result < 0) {
    LOG(ERROR) << "Failed to epoll_wait: " << strerror(errno);
    return ZX_ERR_IO;
  }
  if (result == 0)
    return ZX_ERR_TIMED_OUT;

  static_cast<EpollHandler*>(ev.data.ptr)->on_epoll_event(ev.events);
  return ZX_OK;
}

void EpollReader::Wakeup() {
  int err;
  do {
    err = eventfd_write(wakeup_fd_.get(), 1ull);
  } while (err == -1 && errno == EINTR);
  DCHECK_EQ(err, 0);
}

void EpollReader::on_epoll_event(uint32_t events) {
  eventfd_t value;
  int err;
  do {
    err = eventfd_read(wakeup_fd_.get(), &value);
  } while (err < 0 && errno == EINTR);
  DCHECK_EQ(err, 0);
}
