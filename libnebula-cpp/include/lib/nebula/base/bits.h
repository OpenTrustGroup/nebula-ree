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

#include <cinttypes>
#include <limits>

namespace nebula {
namespace base {

static inline int64_t SignedSaturatedAdd64(int64_t lhs, int64_t rhs) {
  using limits = std::numeric_limits<int64_t>;
  // underflow if |lhs + rhs < min|.
  if (lhs < 0 && rhs < limits::min() - lhs)
    return limits::min();
  // overflow if |lhs + rhs > max|.
  if (lhs > 0 && rhs > limits::max() - lhs)
    return limits::max();
  return lhs + rhs;
}

}  // namespace base
}  // namespace nebula