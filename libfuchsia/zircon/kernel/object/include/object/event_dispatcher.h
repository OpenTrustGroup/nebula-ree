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

#include <sys/types.h>
#include <zircon/types.h>

#include <object/dispatcher.h>
#include <object/event.h>
#include <object/state_observer.h>

#include "lib/nebula/base/unique_fd.h"

class EventDispatcher final : public SoloDispatcher, public StateObserver {
 public:
  static zx_status_t Create(uint32_t options, std::shared_ptr<EventDispatcher>* dispatcher,
                            zx_rights_t* rights);
  static zx_status_t Create(uint32_t options, std::shared_ptr<EventDispatcher>* dispatcher,
                            zx_rights_t* rights, int fd);

  ~EventDispatcher() final;

  zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_EVENT; }
  bool has_state_tracker() const final { return true; }
  zx_signals_t allowed_user_signals() const final { return ZX_USER_SIGNAL_ALL | ZX_EVENT_SIGNALED; }
  int get_fd() const final { return eventfd_; }
  void on_epoll_event(uint32_t events) final;

  virtual Flags OnInitialize(zx_signals_t initial_state, const CountInfo* cinfo);
  virtual Flags OnStateChange(zx_signals_t new_state);
  virtual Flags OnCancel(const Handle* handle);

  zx_status_t Write();

 private:
  explicit EventDispatcher(uint32_t options);
  void SignalEventLocked();
  void UnsignalEventLocked();
  nebula::base::unique_fd eventfd_;
  bool signaled_;
};