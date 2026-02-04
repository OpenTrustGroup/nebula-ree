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

#include <sys/epoll.h>
#include <zircon/types.h>

#include <functional>
#include <mutex>
#include <unordered_map>

#include <object/epoll_handler.h>

#include "lib/nebula/base/mutex.h"
#include "lib/nebula/base/thread_annotations.h"
#include "lib/nebula/base/unique_fd.h"

// #if !defined(EFD_SEMAPHORE)
// #define EFD_SEMAPHORE (1 << 0)
// #endif

class EpollReader final : public EpollHandler {
 public:
  EpollReader();

  void Watch(int fd, EpollHandler* handler, uint32_t events = EPOLLIN | EPOLLET);
  void Unwatch(int fd);

  zx_status_t WaitAndDispatch(int timeout_ms);

  // Wakeup the reader if it blocks at |WaitAndDispatch|.
  void Wakeup();

  int get_fd() const { return epoll_fd_.get(); }

 private:
  void on_epoll_event(uint32_t events) final;

  bool increment_watcher_count_locked(int fd);
  bool decrement_watcher_count_locked(int fd);

  // There could be multiple watchers with same fd, only add/remove to/from the epoll_set for the
  // first and last watcher.
  nebula::base::Mutex mu_;
  std::unordered_map<int, size_t> watcher_count_ GUARDED_BY(mu_);

  nebula::base::unique_fd epoll_fd_;
  nebula::base::unique_fd wakeup_fd_;
};