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

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/epoll.h>
#include <trusty_ipc.h>

#include <object/tipc_chan_dispatcher.h>

#include "lib/nebula/base/auto_lock.h"
#include "lib/nebula/base/logging.h"

// static
zx_status_t TipcChanDispatcher::Create(std::shared_ptr<TipcChanDispatcher>* dispatcher0,
                                       std::shared_ptr<TipcChanDispatcher>* dispatcher1,
                                       zx_rights_t* rights) {
  nebula::base::unique_fd sock[2];
  nebula::base::Socketpair(SOCK_SEQPACKET, &sock[0], &sock[1]);

  if (!sock[0].ok() || !sock[1].ok())
    return ZX_ERR_IO;

  if (fcntl(sock[0].get(), F_SETFL, O_NONBLOCK) < 0)
    return ZX_ERR_IO;

  if (fcntl(sock[1].get(), F_SETFL, O_NONBLOCK) < 0)
    return ZX_ERR_IO;

  auto holder0 = std::make_shared<PeerHolder<TipcChanDispatcher>>();
  if (!holder0)
    return ZX_ERR_NO_MEMORY;
  auto holder1 = holder0;

  std::shared_ptr<TipcChanDispatcher> ch0(new TipcChanDispatcher(holder0));
  if (!ch0)
    return ZX_ERR_NO_MEMORY;

  std::shared_ptr<TipcChanDispatcher> ch1(new TipcChanDispatcher(holder1));
  if (!ch1)
    return ZX_ERR_NO_MEMORY;

  ch0->Init(std::move(sock[0]), ch1, ChanBackend::kUnixSock);
  ch1->Init(std::move(sock[1]), ch0, ChanBackend::kUnixSock);

  *dispatcher0 = ch0;
  *dispatcher1 = ch1;

  return ZX_OK;
}

// static
zx_status_t TipcChanDispatcher::CreateFromFileDescriptor(
    nebula::base::unique_fd fd, ChanBackend backend,
    std::shared_ptr<TipcChanDispatcher>* dispatcher, zx_rights_t* rights) {
  if (!fd.ok())
    return ZX_ERR_IO;

  if (fcntl(fd.get(), F_SETFL, O_NONBLOCK) < 0)
    return ZX_ERR_IO;

  auto holder = std::make_shared<PeerHolder<TipcChanDispatcher>>();
  if (!holder)
    return ZX_ERR_NO_MEMORY;

  std::shared_ptr<TipcChanDispatcher> ch(new TipcChanDispatcher(holder));
  if (!ch)
    return ZX_ERR_NO_MEMORY;

  ch->Init(std::move(fd), /*peer=*/nullptr, backend);
  *dispatcher = ch;

  return ZX_OK;
}

TipcChanDispatcher::TipcChanDispatcher(std::shared_ptr<PeerHolder<TipcChanDispatcher>> holder)
    : PeeredDispatcher(std::move(holder), IPC_HANDLE_POLL_NONE), backend_(ChanBackend::kNone) {}

TipcChanDispatcher::~TipcChanDispatcher() {}

void TipcChanDispatcher::Init(nebula::base::unique_fd chanfd,
                              std::shared_ptr<TipcChanDispatcher> peer, ChanBackend backend) {
  DCHECK(chanfd.ok());
  chanfd_ = std::move(chanfd);
  peer_ = peer;
  backend_ = backend;
}

void TipcChanDispatcher::OnPeerZeroHandlesLocked() {
  peer_.reset();
  UpdateStateLocked(0u, IPC_HANDLE_POLL_HUP);
}

void TipcChanDispatcher::on_zero_handles() {
  nebula::base::AutoLock lock(get_lock());

  std::shared_ptr<TipcChanDispatcher> other = std::move(peer_);
  if (other)
    other->OnPeerZeroHandlesLocked();
}

zx_status_t TipcChanDispatcher::Read(void* bytes, zx_handle_t* handles, uint32_t num_bytes,
                                     uint32_t num_handles, uint32_t* actual_bytes,
                                     uint32_t* actual_handles) {
  if (handles || num_handles || actual_handles)
    return ZX_ERR_INVALID_ARGS;

  nebula::base::AutoLock lock(get_lock());

  int rc = read(chanfd_.get(), bytes, num_bytes);
  if (rc < 0) {
    if (errno == EAGAIN) {
      UpdateStateLocked(IPC_HANDLE_POLL_MSG, 0u);
      return ZX_ERR_SHOULD_WAIT;
    }

    LOG(ERROR) << "Failed to read channel: " << strerror(errno);
    return ZX_ERR_IO;
  }

  if (actual_bytes)
    *actual_bytes = rc;

  if (rc == 0 && num_bytes > 0) {
    UpdateStateLocked(0u, IPC_HANDLE_POLL_HUP);
    return ZX_ERR_PEER_CLOSED;
  }

  if (backend_ == ChanBackend::kUnixSock) {
    char ch;
    if (recv(chanfd_.get(), &ch, 1, MSG_PEEK) <= 0) {
      // EOF or error.
      UpdateStateLocked(IPC_HANDLE_POLL_MSG, 0u);
    }
  }

  return ZX_OK;
}

zx_status_t TipcChanDispatcher::Write(const void* bytes, uint32_t num_bytes,
                                      const zx_handle_t* handles, uint32_t num_handles) {
  if (handles || num_handles)
    return ZX_ERR_INVALID_ARGS;

  nebula::base::AutoLock lock(get_lock());
  if (!peer_ && backend_ == ChanBackend::kUnixSock)
    return ZX_ERR_PEER_CLOSED;

  int rc = write(chanfd_.get(), bytes, num_bytes);
  if (rc < 0) {
    if (errno == EAGAIN)
      return ZX_ERR_SHOULD_WAIT;
    LOG(ERROR) << "Failed to write channel: " << strerror(errno);
    return ZX_ERR_IO;
  }

  if (peer_)
    peer_->UpdateStateLocked(0u, IPC_HANDLE_POLL_MSG);

  return ZX_OK;
}

void TipcChanDispatcher::on_epoll_event(uint32_t events) {
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
