/*
 * Copyright (C) 2021 GoldenRiver Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <errno.h>
#include <poll.h>
#include <string.h>

#include "lib/nebula/base/logging.h"
#include "lib/nebula/base/macros.h"
#include "lib/nebula/base/unique_fd.h"

namespace nebula {
namespace base {

class SelfPipe {
 public:
  SelfPipe() {
    bool success = nebula::base::Pipe(&read_pipe, &write_pipe, O_NONBLOCK);
    CHECK(success) << "Failed to create pipe: " << strerror(errno);
  }

  ~SelfPipe() { Cancel(); }

  int WaitReadable(int fd) { return WaitReadableForMilliseconds(fd, -1); }

  int WaitReadableForMilliseconds(int fd, long timeout_ms) {
    struct pollfd events[] = {
        {.fd = read_pipe.get(), .events = POLLIN},
        {.fd = fd, .events = POLLIN},
    };
    int rc = poll(events, 2, timeout_ms);
    if (rc < 0)
      return -errno;
    if (rc == 0)
      return -ETIMEDOUT;
    if (events[1].revents & POLLNVAL)
      return -EINVAL;
    if (events[0].revents & POLLIN)
      return -EINTR;

    CHECK(events[1].revents & POLLIN);
    return 1;
  }

  int Cancel() {
    int rc = write(write_pipe.get(), "x", 1);
    if (rc < 0)
      return -errno;
    return 0;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SelfPipe);

  nebula::base::unique_fd read_pipe;
  nebula::base::unique_fd write_pipe;
};

}  // namespace base
}  // namespace nebula