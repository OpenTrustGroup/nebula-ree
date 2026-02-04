// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <lib/zx/tipc_chan.h>
#include <trusty_uuid.h>
#include <zircon/types.h>

namespace nebula {
namespace ipc {

class PortMessageHandler {
 public:
  virtual ~PortMessageHandler();

  virtual zx_status_t OnNewChannel(uuid_t uuid, zx::tipc_chan chan) = 0;

  virtual void OnPortGone();
};

}  // namespace ipc
}  // namespace nebula