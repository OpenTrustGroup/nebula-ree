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

#include <object/unix_socket_channel_dispatcher.h>

static constexpr int kMaxFds = 100;

zx_status_t UnixSocketChannelDispatcher::Create(
    int fd, std::shared_ptr<UnixSocketChannelDispatcher> *dispatcher) {
  std::shared_ptr<UnixSocketChannelDispatcher> disp(new UnixSocketChannelDispatcher(fd));
  if (!disp)
    return ZX_ERR_NO_MEMORY;

  *dispatcher = disp;
  return ZX_OK;
}

zx_status_t UnixSocketChannelDispatcher::Create(
    std::shared_ptr<UnixSocketChannelDispatcher> *dispatcher) {
  std::shared_ptr<UnixSocketChannelDispatcher> disp(new UnixSocketChannelDispatcher());
  if (!disp)
    return ZX_ERR_NO_MEMORY;

  *dispatcher = disp;
  return ZX_OK;
}

UnixSocketChannelDispatcher::UnixSocketChannelDispatcher(int fd) { sockfd_.reset(fd); }
UnixSocketChannelDispatcher::~UnixSocketChannelDispatcher() {}

zx_status_t UnixSocketChannelDispatcher::Connect(const char *path) {
  struct sockaddr_un un;
  int ret;

  if (path == nullptr || strlen(path) == 0) {
    return ZX_ERR_INVALID_ARGS;
  }

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return ZX_ERR_INTERNAL;
  }

  memset(&un, 0, sizeof(un));
  un.sun_family = AF_UNIX;
  strncpy(un.sun_path, path, sizeof(un.sun_path));
  un.sun_path[sizeof(un.sun_path) - 1] = '\0';
  ret = connect(fd, (struct sockaddr *)&un, sizeof(un));
  if (ret < 0) {
    return ZX_ERR_IO;
  }

  sockfd_.reset(fd);
  return ZX_OK;
}

zx_status_t UnixSocketChannelDispatcher::Read(char *buf, int buf_len, int *fds, int max_fds,
                                              uint32_t *read_len, uint32_t *fd_num) {
  if (max_fds > kMaxFds) {
    return ZX_ERR_INVALID_ARGS;
  }

  struct iovec iov;
  struct msghdr msgh;
  char control[CMSG_SPACE(max_fds * sizeof(int))];
  struct cmsghdr *cmsg;
  int got_fds = 0;
  int ret;

  nebula::base::AutoLock lock(get_lock());

  if (fd_num) {
    *fd_num = 0;
  }

  memset(&msgh, 0, sizeof(msgh));
  iov.iov_base = buf;
  iov.iov_len = buf_len;

  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;
  msgh.msg_control = control;
  msgh.msg_controllen = sizeof(control);

  ret = recvmsg(sockfd_.get(), &msgh, 0);
  if (ret <= 0) {
    return ZX_ERR_IO;
  }

  if (msgh.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) {
    return ZX_ERR_IO;
  }

  if (fds && max_fds > 0) {
    for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
      if ((cmsg->cmsg_level == SOL_SOCKET) && (cmsg->cmsg_type == SCM_RIGHTS)) {
        got_fds = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
        if (fd_num) {
          *fd_num = got_fds;
        }
        memcpy(fds, CMSG_DATA(cmsg), got_fds * sizeof(int));
        break;
      }
    }

    /* Clear out unused file descriptors */
    while (got_fds < max_fds)
      fds[got_fds++] = -1;
  }

  if (read_len) {
    *read_len = ret;
  }

  UpdateStateLocked(IPC_HANDLE_POLL_MSG, 0u);

  return ZX_OK;
}

zx_status_t UnixSocketChannelDispatcher::Write(const char *buf, int buflen, int *fds, int fd_num,
                                               uint32_t *written_len) {
  struct iovec iov;
  struct msghdr msgh;
  size_t fdsize = fd_num * sizeof(int);
  char control[CMSG_SPACE(fdsize)];
  struct cmsghdr *cmsg;
  int ret;

  nebula::base::AutoLock lock(get_lock());

  memset(&msgh, 0, sizeof(msgh));
  iov.iov_base = (void *)buf;
  iov.iov_len = buflen;

  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;

  if (fds && fd_num > 0) {
    msgh.msg_control = control;
    msgh.msg_controllen = sizeof(control);
    cmsg = CMSG_FIRSTHDR(&msgh);
    if (cmsg == NULL) {
      errno = EINVAL;
      return ZX_ERR_IO;
    }
    cmsg->cmsg_len = CMSG_LEN(fdsize);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    memcpy(CMSG_DATA(cmsg), fds, fdsize);
  } else {
    msgh.msg_control = NULL;
    msgh.msg_controllen = 0;
  }

  do {
    ret = sendmsg(sockfd_.get(), &msgh, MSG_NOSIGNAL);
  } while (ret < 0 && errno == EINTR);

  if (ret < 0) {
    return ZX_ERR_IO;
  }

  if (written_len) {
    *written_len = ret;
  }
  return ZX_OK;
}

void UnixSocketChannelDispatcher::on_epoll_event(uint32_t events) {
  nebula::base::AutoLock lock(get_lock());

  uint32_t state = 0u;
  if (events & EPOLLIN)
    state |= IPC_HANDLE_POLL_MSG;
  if (events & EPOLLERR)
    state |= IPC_HANDLE_POLL_ERROR;
  if (events & EPOLLHUP)
    state |= IPC_HANDLE_POLL_HUP;

  UpdateStateLocked(0u, state);
}
