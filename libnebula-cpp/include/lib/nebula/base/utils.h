// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 GoldenRiver Technology Co., Ltd.

#pragma once

#include <lib/nebula/base/logging.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

namespace nebula {
namespace base {

template <typename T>
static inline std::string ToBytes(T* ptr) {
  static_assert(std::is_pod<T>::value, "");
  std::string result;
  result.resize(sizeof(*ptr));
  std::memcpy(&result[0], ptr, sizeof(*ptr));
  return result;
}

template <typename T>
static inline std::string ToBytes(T obj) {
  return ToBytes(&obj);
}

template <typename T>
static inline T* ToObject(std::string str, T* ptr) {
  static_assert(std::is_pod<T>::value, "");
  CHECK_EQ(sizeof(T), str.size());
  return (T*)std::memcpy((void*)ptr, &str[0], sizeof(T));
}

namespace {
class ObjectConverter {
 public:
  ObjectConverter(std::string str) : str_(std::move(str)) {}

  template <typename ObjectType>
  operator ObjectType() const {
    static_assert(std::is_pod<ObjectType>::value, "");
    CHECK_GE(sizeof(ObjectType), str_.size());
    ObjectType object;
    std::memcpy(&object, &str_[0], sizeof(ObjectType));
    return object;
  }

 private:
  std::string str_;
};
}  // namespace

static inline ObjectConverter ToObject(std::string str) { return ObjectConverter(std::move(str)); }

}  // namespace base
}  // namespace nebula