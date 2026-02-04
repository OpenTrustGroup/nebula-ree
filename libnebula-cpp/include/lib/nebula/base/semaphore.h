/*
 * Copyright (C) 2021 GoldenRiver Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <semaphore.h>

#include <chrono>
#include <limits>

#include "lib/nebula/base/logging.h"
#include "lib/nebula/base/bits.h"
#include "lib/nebula/base/time.h"

namespace nebula {
namespace base {

class Semaphore {
 public:
  explicit Semaphore(unsigned int initial_count) {
    int result = sem_init(&sem_, 0, initial_count);
    DCHECK_EQ(0, result);
    UNUSED(result);
  }

  ~Semaphore() {
    int result = sem_destroy(&sem_);
    DCHECK_EQ(0, result);
    UNUSED(result);
  }

  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  // Increments the semaphore counter.
  void Post() {
    int result = sem_post(&sem_);
    if (result != 0) {
      LOG(FATAL) << "Error when signaling semaphore, errno: " << errno;
    }
  }

  // Decrements the semaphore counter if it is positive, or blocks until it
  // becomes positive and then decrements the counter.
  void Wait() {
    while (true) {
      int result = sem_wait(&sem_);
      if (result == 0)
        return;
      // Signal caused spurious wakeup.
      DCHECK_EQ(-1, result);
      DCHECK_EQ(EINTR, errno);
    }
  }

  // Try to decrements the semaphore counter, and retuns immediately. If counter
  // is positive and decrements successfully, returns true, otherwise false.
  bool TryWait() {
    while (true) {
      int result = sem_trywait(&sem_);
      if (result == 0)
        return true;
      if (result == -1 && errno == EAGAIN)
        return false;
      // Signal caused spurious wakeup.
      DCHECK_EQ(-1, result);
      DCHECK_EQ(EINTR, errno);
    }
  }

  // Like Wait() but returns after |timeout| has passed. If the timeout
  // happens the return value is false and the counter is unchanged. Otherwise
  // the semaphore counter is decremented and true is returned.
  // bool WaitForNanoseconds(long timeout_ns);
  template <typename Rep, typename Period>
  bool WaitWithTimeout(const std::chrono::duration<Rep, Period>& timeout) {
    return WaitWithDeadline(std::chrono::nanoseconds(SignedSaturatedAdd64(
        std::chrono::system_clock::now().time_since_epoch().count(),
        std::chrono::duration_cast<std::chrono::nanoseconds>(timeout).count())));
  }

  // Like Wait() but returns until |deadline_ns| has reached. If the timeout
  // happens the return value is false and the counter is unchanged. Otherwise
  // the semaphore counter is decremented and true is returned.
  template <typename Rep, typename Period>
  bool WaitWithDeadline(const std::chrono::duration<Rep, Period>& deadline) {
    timespec abstime = ChronoToTimespec(deadline);
    while (true) {
      int result = sem_timedwait(&sem_, &abstime);
      if (result == 0)
        return true;
      if (result == -1 && errno == ETIMEDOUT)
        return false;
      // Signal caused spurious wakeup.
      DCHECK_EQ(-1, result);
      DCHECK_EQ(EINTR, errno);
    }
  }

 private:
  sem_t sem_;
};

}  // namespace base
}  // namespace nebula