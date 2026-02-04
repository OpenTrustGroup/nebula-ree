// Copyright 2021 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <zircon/types.h>

#include <object/dispatcher.h>

#include "lib/nebula/base/unique_fd.h"

class UnixSocketChannelDispatcher : public SoloDispatcher {
 public:
  static zx_status_t Create(int fd, std::shared_ptr<UnixSocketChannelDispatcher> *dispatcher);
  static zx_status_t Create(std::shared_ptr<UnixSocketChannelDispatcher> *dispatcher);
  ~UnixSocketChannelDispatcher();

  zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_UNIX_SOCK_CHAN; }
  bool has_state_tracker() const final { return true; }

  zx_status_t Connect(const char *path);
  zx_status_t Read(char *buf, int buf_len, int *fds, int max_fds, uint32_t *read_len,
                   uint32_t *fd_num);
  zx_status_t Read(char *buf, int buf_len, uint32_t *read_len);
  zx_status_t Write(const char *buf, int buflen, int *fds, int fd_num, uint32_t *written_len);

  // Overrides |Dispatcher|.
  int get_fd() const final { return sockfd_.get(); }
  void on_epoll_event(uint32_t events) final;

 private:
  explicit UnixSocketChannelDispatcher(int fd);
  UnixSocketChannelDispatcher() {}
  zx_status_t Init();

  std::string path_;
  nebula::base::unique_fd sockfd_;
};