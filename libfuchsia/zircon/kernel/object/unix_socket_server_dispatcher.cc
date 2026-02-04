// Copyright 2021 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <trusty_ipc.h>

#include <object/unix_socket_server_dispatcher.h>

zx_status_t UnixSocketServerDispatcher::Create(
    const char *path, std::shared_ptr<UnixSocketServerDispatcher> *dispatcher) {
  std::shared_ptr<UnixSocketServerDispatcher> disp(new UnixSocketServerDispatcher(path));
  if (!disp)
    return ZX_ERR_NO_MEMORY;

  zx_status_t status = disp->Init();
  if (status != ZX_OK) {
    return status;
  }

  *dispatcher = disp;
  return ZX_OK;
}

UnixSocketServerDispatcher::UnixSocketServerDispatcher(const char *path) : path_(path) {}
UnixSocketServerDispatcher::~UnixSocketServerDispatcher() { unlink(path_.c_str()); }

zx_status_t UnixSocketServerDispatcher::Init() {
  struct sockaddr_un un;
  int ret;

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return ZX_ERR_INTERNAL;
  }

  unlink(path_.c_str());
  memset(&un, 0, sizeof(un));
  un.sun_family = AF_UNIX;
  strncpy(un.sun_path, path_.c_str(), sizeof(un.sun_path));
  un.sun_path[sizeof(un.sun_path) - 1] = '\0';

  ret = bind(fd, (struct sockaddr *)&un, sizeof(un));
  if (ret < 0) {
    return ZX_ERR_INTERNAL;
  }

  ret = listen(fd, /* backlog= */128);
  if (ret < 0) {
    return ZX_ERR_INTERNAL;
  }

  sockfd_.reset(fd);
  return ZX_OK;
}

zx_status_t UnixSocketServerDispatcher::Accept(
    std::shared_ptr<UnixSocketChannelDispatcher> *dispatcher) {
  int fd = accept(sockfd_.get(), NULL, NULL);
  if (fd < 0) {
    return ZX_ERR_INTERNAL;
  }

  {
    nebula::base::AutoLock lock(get_lock());
    UpdateStateLocked(IPC_HANDLE_POLL_READY, 0u);
  }

  return UnixSocketChannelDispatcher::Create(fd, dispatcher);
}

void UnixSocketServerDispatcher::on_epoll_event(uint32_t events) {
  nebula::base::AutoLock lock(get_lock());

  uint32_t state = 0u;
  if (events & EPOLLIN)
    state |= IPC_HANDLE_POLL_READY;
  if (events & EPOLLERR)
    state |= IPC_HANDLE_POLL_ERROR;
  if (events & EPOLLHUP)
    state |= IPC_HANDLE_POLL_HUP;

  UpdateStateLocked(0u, state);
}