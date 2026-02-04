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

#include "lib/nebula/base/thread_annotations.h"

// Notes about class NullLock
//
// NullLock is a stub class which exposes the same API as fbl::Mutex, but does
// nothing (it neither allocates nor locks).  It may be used with fbl templated
// utility classes whose locking behavior is determined by passing a lock class
// type as a template parameter to get a no-locking behavior.
namespace nebula {
namespace base {

class CAPABILITY("mutex") NullLock {
 public:
  constexpr NullLock() {}
  void lock() ACQUIRE() {}
  void unlock() RELEASE() {}
};

}  // namespace base
}  // namespace nebula