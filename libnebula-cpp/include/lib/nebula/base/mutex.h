// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/compiler.h>
#include <zircon/types.h>

#include <mutex>

#include "lib/nebula/base/macros.h"
#include "lib/nebula/base/thread_annotations.h"

namespace nebula {
namespace base {

class CAPABILITY("mutex") Mutex {
 public:
  constexpr Mutex() = default;
  void Acquire() ACQUIRE() { mutex_.lock(); }
  void Release() RELEASE() { mutex_.unlock(); }

  /* IsHeld is not supported by the Mutex wrapper in user-mode as C11 mtx_t
   * instances do not support a direct IsHeld style check.  A possible
   * implementation could be built out of mtx_trylock, but would require
   * either relaxing away the const constraint on the method signature, or
   * flagging the mutex_ member as mutable */

  std::mutex* GetInternal() RETURN_CAPABILITY(mutex_) { return &mutex_; }

  // suppress default constructors
  DISALLOW_COPY_ASSIGN_AND_MOVE(Mutex);

 private:
  std::mutex mutex_;
};

}  // namespace base
}  // namespace nebula