// Copyright 2021 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <zircon/types.h>

#include <object/dispatcher.h>

#include "lib/nebula/base/unique_fd.h"
#include "unix_socket_channel_dispatcher.h"

class UnixSocketServerDispatcher : public SoloDispatcher {
 public:
  static zx_status_t Create(const char *path,
                            std::shared_ptr<UnixSocketServerDispatcher> *dispatcher);
  ~UnixSocketServerDispatcher();

  zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_UNIX_SOCK; }
  bool has_state_tracker() const final { return true; }

  zx_status_t Accept(std::shared_ptr<UnixSocketChannelDispatcher> *dispatcher);

  // Overrides |Dispatcher|.
  int get_fd() const final { return sockfd_.get(); }
  void on_epoll_event(uint32_t events) final;

 private:
  explicit UnixSocketServerDispatcher(const char *path);
  zx_status_t Init();

  std::string path_;
  nebula::base::unique_fd sockfd_;
};