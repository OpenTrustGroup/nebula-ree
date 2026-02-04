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

#include <memory>
#include <unordered_set>

#include <object/dispatcher.h>

class Handle {
 public:
  const std::shared_ptr<Dispatcher>& dispatcher() const { return dispatcher_; }

  static Handle* Make(std::shared_ptr<Dispatcher> dispatcher);

  static Handle* GetHandleLocked(zx_handle_t handle);

  static zx_handle_t ToU64(Handle* handle);
  static Handle* FromU64(uint64_t value);

  ~Handle();

 private:
  DISALLOW_COPY_ASSIGN_AND_MOVE(Handle);

  Handle(std::shared_ptr<Dispatcher> dispatcher);

  std::shared_ptr<Dispatcher> dispatcher_;
};