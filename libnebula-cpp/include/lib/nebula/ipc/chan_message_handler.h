// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <lib/zx/tipc_chan.h>
#include <zircon/types.h>

#include <string>

namespace nebula {
namespace ipc {

class ChanMessageHandler {
 public:
  virtual ~ChanMessageHandler();

  virtual zx_status_t OnRawMessage(std::string bytes) = 0;

  virtual void OnChannelGone();
};

}  // namespace ipc
}  // namespace nebula