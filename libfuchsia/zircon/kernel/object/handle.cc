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

#include <object/handle.h>

static std::mutex mu_;
static std::unordered_set<zx_handle_t> handle_set_;

static std::mutex g_handle_table_lock;
std::mutex* GetHandleTableLock() { return &g_handle_table_lock; }

// static
Handle* Handle::Make(std::shared_ptr<Dispatcher> dispatcher) {
  std::unique_lock<std::mutex> lock(mu_);
  dispatcher->increment_handle_count();
  Handle* handle = new Handle(std::move(dispatcher));
  handle_set_.insert(ToU64(handle));
  return handle;
}

Handle::Handle(std::shared_ptr<Dispatcher> dispatcher) : dispatcher_(dispatcher) {}

Handle::~Handle() {
  if (dispatcher_->has_state_tracker())
    dispatcher_->Cancel(this);

  bool zero_handles = false;
  {
    std::unique_lock<std::mutex> lock(mu_);
    zero_handles = dispatcher_->decrement_handle_count();
    handle_set_.erase(ToU64(this));
  }

  if (zero_handles) {
    dispatcher_->on_zero_handles();
  }
}

Handle* Handle::GetHandleLocked(zx_handle_t handle) { return FromU64(handle); }

zx_handle_t Handle::ToU64(Handle* handle) { return reinterpret_cast<zx_handle_t>(handle); }

Handle* Handle::FromU64(uint64_t value) {
  std::unique_lock<std::mutex> lock(mu_);
  if (handle_set_.find(value) == handle_set_.end())
    return nullptr;
  return reinterpret_cast<Handle*>(value);
}