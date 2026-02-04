// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include "lib/nebula/ipc/port_message_handler.h"

namespace nebula {
namespace ipc {

PortMessageHandler::~PortMessageHandler() = default;

void PortMessageHandler::OnPortGone() {}

}  // namespace ipc
}  // namespace nebula