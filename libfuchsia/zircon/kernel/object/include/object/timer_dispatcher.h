// Copyright 2017 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <zircon/types.h>

#include <object/dispatcher.h>

#include "lib/nebula/base/unique_fd.h"

class TimerDispatcher final : public SoloDispatcher {
 public:
  static zx_status_t Create(uint32_t options, std::shared_ptr<TimerDispatcher>* dispatcher,
                            zx_rights_t* rights);

  ~TimerDispatcher();

  zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_TIMER; }
  bool has_state_tracker() const final { return true; }
  void on_zero_handles() final;

  // Timer specific ops.
  zx_status_t Set(zx_time_t deadline, zx_duration_t slack);
  zx_status_t Cancel();

  int get_fd() const final { return tfd_.get(); }
  void on_epoll_event(uint32_t events) final;

 private:
  explicit TimerDispatcher();
  void SetTimerLocked(bool cancel_first) REQUIRES(get_lock());
  bool CancelTimerLocked() REQUIRES(get_lock());

  zx_time_t deadline_ GUARDED_BY(get_lock());
  zx_duration_t slack_ GUARDED_BY(get_lock());
  bool cancel_pending_ GUARDED_BY(get_lock());
  nebula::base::unique_fd tfd_;
};
