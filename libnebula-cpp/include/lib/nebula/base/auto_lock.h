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

#include <mutex>

#include "lib/nebula/base/macros.h"
#include "lib/nebula/base/mutex.h"
#include "lib/nebula/base/null_lock.h"
#include "lib/nebula/base/thread_annotations.h"

namespace nebula {
namespace base {

class SCOPED_CAPABILITY AutoLock {
 public:
  explicit AutoLock(std::mutex* mutex) ACQUIRE(mutex) : mutex_(mutex) { mutex_->lock(); }

  explicit AutoLock(Mutex* mutex) ACQUIRE(mutex) : AutoLock(mutex->GetInternal()) {}

  explicit AutoLock(NullLock* mutex) ACQUIRE(mutex) : mutex_(nullptr) {}

  ~AutoLock() RELEASE() { release(); }

  // early release the mutex before the object goes out of scope
  void release() RELEASE() {
    if (mutex_) {
      mutex_->unlock();
      mutex_ = nullptr;
    }
  }

  // suppress default constructors
  DISALLOW_COPY_ASSIGN_AND_MOVE(AutoLock);

 private:
  std::mutex* mutex_;
};

}  // namespace base
}  // namespace nebula