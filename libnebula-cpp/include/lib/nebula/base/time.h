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

#include <chrono>
#include <cstdint>
#include <ctime>

namespace nebula {
namespace base {

constexpr int64_t kNanosPerNanosecond = 1;
constexpr int64_t kNanosPerMicrosecond = 1000;
constexpr int64_t kNanosPerMillisecond = 1000 * 1000;
constexpr int64_t kNanosPerSecond = 1000 * 1000 * 1000;

/*
 * Copied/inspired from absl/time/time.h
 */
static inline timespec __MakeNormalizedTimespec(int64_t sec, int64_t ticks) {
  return (ticks < 0) ? timespec{sec - 1, ticks + kNanosPerSecond} : timespec{sec, ticks};
}

// IsValidRep64<T>(0) is true if the expression `int64_t{std::declval<T>()}` is
// valid. That is, if a T can be assigned to an int64_t without narrowing.
template <typename T>
static inline constexpr auto __IsValidRep64(int) -> decltype(int64_t{std::declval<T>()} == 0) {
  return true;
}

template <std::intmax_t N>
static inline constexpr timespec __FromInt64(int64_t v, std::ratio<1, N>) {
  static_assert(0 < N && N <= 1000 * 1000 * 1000, "Unsupported ratio");
  // Subsecond ratios cannot overflow.
  return __MakeNormalizedTimespec(v / N, v % N * kNanosPerNanosecond * 1000 * 1000 * 1000 / N);
}

// Converts a std::chrono::duration to an unix timespec.
template <typename Rep, typename Period>
static inline constexpr timespec ChronoToTimespec(const std::chrono::duration<Rep, Period>& d) {
  static_assert(__IsValidRep64<Rep>(0), "duration::rep is invalid");
  return __FromInt64(int64_t{d.count()}, Period{});
}

static inline int64_t GetCurrentTimeNanosFromSystem() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now() - std::chrono::system_clock::from_time_t(0))
      .count();
}

}  // namespace base
}  // namespace nebula