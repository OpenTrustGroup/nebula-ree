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

#include <string.h>
#include <sys/timerfd.h>

#include <object/common.h>
#include <object/timer_dispatcher.h>

#include "lib/nebula/base/auto_lock.h"

namespace {

static int _timerfd_update(int tfd, zx_time_t deadline) {
  // Clear pending events
  uint64_t unused;
  read(tfd, &unused, sizeof(unused));

  struct itimerspec new_ts = {};
  new_ts.it_value.tv_sec = deadline / ZX_SEC(1);
  new_ts.it_value.tv_nsec = deadline % ZX_SEC(1);
  return timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_ts, /*old_ts=*/nullptr);
}

static int timer_set(int tfd, zx_time_t deadline) {
  int rc = _timerfd_update(tfd, deadline);
  if (rc < 0)
    LOG(ERROR) << "Failed to set timer: " << strerror(errno);
  return rc;
}

static bool timer_cancel(int tfd) {
  int rc = _timerfd_update(tfd, 0u);
  if (rc < 0)
    LOG(ERROR) << "Failed to cancel timer: " << strerror(errno);
  return rc == 0;
}

}  // namespace

TimerDispatcher::TimerDispatcher() : deadline_(0u), slack_(0u), cancel_pending_(false) {}

TimerDispatcher::~TimerDispatcher() { DCHECK(deadline_ == 0u); }

void TimerDispatcher::on_zero_handles() {
  // The timers can be kept alive indefinitely by the callbacks, so
  // we need to cancel when there are no more user-mode clients.
  nebula::base::AutoLock lock(get_lock());

  // We must ensure that the timer callback (running in interrupt context,
  // possibly on a different CPU) has completed before possibly destroy
  // the timer.  So cancel the timer if we haven't already.
  if (!CancelTimerLocked())
    timer_cancel(tfd_.get());
}

// static
zx_status_t TimerDispatcher::Create(uint32_t options, std::shared_ptr<TimerDispatcher>* dispatcher,
                                    zx_rights_t* rights) {
  std::shared_ptr<TimerDispatcher> timer(new TimerDispatcher);
  if (!timer)
    return ZX_ERR_NO_MEMORY;

  int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (tfd < 0) {
    LOG(ERROR) << "Failed to create timer: " << errno;
    return ZX_ERR_INTERNAL;
  }

  timer->tfd_.reset(tfd);
  *dispatcher = timer;
  return ZX_OK;
}

zx_status_t TimerDispatcher::Set(zx_time_t deadline, zx_duration_t slack) {
  nebula::base::AutoLock lock(get_lock());

  bool did_cancel = CancelTimerLocked();

  // If the timer is already due, then we can set the signal immediately without
  // starting the timer.
  if ((deadline == 0u) || (deadline <= current_time())) {
    UpdateStateLocked(0u, ZX_TIMER_SIGNALED);
    return ZX_OK;
  }

  deadline_ = deadline;
  slack_ = slack;

  // If we're imminently awaiting a timer callback due to a prior cancelation
  // request, let the callback take care of restarting the timer too so
  // everthing happens in the right sequence.
  if (cancel_pending_)
    return ZX_OK;

  // We need to ref-up because the timer and the dpc don't understand
  // refcounted objects. The Release() is called either in OnTimerFired()
  // or in the complicated cancelation path above.
  // AddRef();

  // We must ensure that the timer callback (running in interrupt context,
  // possibly on a different CPU) has completed before set try to set the
  // timer again.  So cancel the timer if we haven't already.
  SetTimerLocked(!did_cancel);

  return ZX_OK;
}
zx_status_t TimerDispatcher::Cancel() {
  nebula::base::AutoLock lock(get_lock());
  CancelTimerLocked();

  return ZX_OK;
}

void TimerDispatcher::on_epoll_event(uint32_t events) {
  nebula::base::AutoLock lock(get_lock());
  timer_cancel(tfd_.get());
  // The timer is firing.
  UpdateStateLocked(0u, ZX_TIMER_SIGNALED);
  deadline_ = 0u;
}

void TimerDispatcher::SetTimerLocked(bool cancel_first) {
  if (cancel_first) {
    timer_cancel(tfd_.get());
  }
  timer_set(tfd_.get(), deadline_);
}
bool TimerDispatcher::CancelTimerLocked() {
  // Always clear the signal bit.
  UpdateStateLocked(ZX_TIMER_SIGNALED, 0u);

  // If the timer isn't pending then we're done.
  if (!deadline_)
    return false;  // didn't call timer_cancel
  deadline_ = 0u;
  slack_ = 0;

  // If we're already waiting for the timer to be canceled, then we don't need
  // to cancel it again.
  if (cancel_pending_)
    return false;  // didn't call timer_cancel

  // The timer is active and needs to be canceled.
  // Refcount is at least 2 because there is a pending timer that we need to
  // cancel.
  bool timer_canceled = timer_cancel(tfd_.get());
  if (timer_canceled) {
    // Managed to cancel before OnTimerFired() ran. So we need to decrement the
    // ref count here.
  } else {
    // The DPC thread is about to run the callback! Yet we are holding the lock.
    // We'll let the timer callback take care of cleanup.
    cancel_pending_ = true;
  }
  return true;  // did call timer_cancel
}