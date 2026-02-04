// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include "lib/nebula/ipc/chan_unblock_waiter.h"

#include <lib/async/default.h>
#include <trusty_ipc.h>

#include <type_traits>

#include "lib/nebula/base/logging.h"

namespace nebula {
namespace ipc {

static constexpr zx_signals_t kSignals = IPC_HANDLE_POLL_SEND_UNBLOCKED;

ChanUnblockWaiter::ChanUnblockWaiter(std::function<void()> message_handler)
    : async_wait_t{{ASYNC_STATE_INIT},
                   &ChanUnblockWaiter::CallHandler,
                   ZX_HANDLE_INVALID,
                   kSignals,
                   0u,
                   {}},
      async_(nullptr),
      message_handler_(std::move(message_handler)) {}

ChanUnblockWaiter::~ChanUnblockWaiter() {
  if (async_)
    async_cancel_wait(async_, this);
}

zx_status_t ChanUnblockWaiter::Bind(zx::tipc_chan chan, async_t* async) {
  if (is_bound())
    Unbind();
  if (!chan)
    return ZX_OK;
  chan_ = std::move(chan);
  if (async) {
    async_ = async;
  } else {
    async_ = async_get_default();
  }
  this->object = chan_.get();
  zx_status_t status = async_begin_wait(async_, this);
  if (status != ZX_OK)
    Unbind();
  return status;
}

zx::tipc_chan ChanUnblockWaiter::Unbind() {
  if (!is_bound())
    return zx::tipc_chan();
  async_cancel_wait(async_, this);
  this->object = ZX_HANDLE_INVALID;
  async_ = nullptr;
  zx::tipc_chan chan = std::move(chan_);
  return chan;
}

// static
async_wait_result_t ChanUnblockWaiter::CallHandler(async_t* async, async_wait_t* wait,
                                                   zx_status_t status,
                                                   const zx_packet_signal_t* signal) {
  return static_cast<ChanUnblockWaiter*>(wait)->OnHandleReady(async, status, signal);
}

async_wait_result_t ChanUnblockWaiter::OnHandleReady(async_t* async, zx_status_t status,
                                                     const zx_packet_signal_t* signal) {
  if (status != ZX_OK) {
    NotifyError();
    return ASYNC_WAIT_FINISHED;
  }

  if (signal->observed & IPC_HANDLE_POLL_SEND_UNBLOCKED) {
    if (message_handler_)
      message_handler_();
    return ASYNC_WAIT_AGAIN;
  }

  NotifyError();
  return ASYNC_WAIT_FINISHED;
}

void ChanUnblockWaiter::NotifyError() {
  Unbind();
  if (error_handler_)
    error_handler_();
}

}  // namespace ipc
}  // namespace nebula
