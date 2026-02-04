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

#include <zircon/syscalls.h>
#include <zircon/types.h>

#include <object/common.h>
#include <object/handle.h>
#include <object/unix_socket_channel_dispatcher.h>
#include <object/unix_socket_server_dispatcher.h>

zx_status_t zx_unix_sock_create(const char* path, zx_handle_t* out) {
  std::shared_ptr<UnixSocketServerDispatcher> sock;
  zx_status_t status = UnixSocketServerDispatcher::Create(path, &sock);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(sock));
  return ZX_OK;
}

zx_status_t zx_unix_sock_accept(zx_handle_t handle, zx_handle_t* out) {
  std::shared_ptr<UnixSocketServerDispatcher> sock;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_READ, &sock);
  if (status != ZX_OK)
    return status;

  std::shared_ptr<UnixSocketChannelDispatcher> chan;
  status = sock->Accept(&chan);
  if (status != ZX_OK) {
    return status;
  }

  *out = Handle::ToU64(Handle::Make(chan));
  return ZX_OK;
}

zx_status_t zx_unix_sock_connect(const char* path, zx_handle_t* out) {
  std::shared_ptr<UnixSocketChannelDispatcher> chan;
  zx_status_t status = UnixSocketChannelDispatcher::Create(&chan);
  if (status != ZX_OK)
    return status;

  status = chan->Connect(path);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(chan));
  return ZX_OK;
}

zx_status_t zx_unix_sock_read(zx_handle_t handle, char* buf, int buf_len, int* fds, int max_fds,
                              uint32_t* read_len, uint32_t* fd_num) {
  std::shared_ptr<UnixSocketChannelDispatcher> chan;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_READ, &chan);
  if (status != ZX_OK)
    return status;

  return chan->Read(buf, buf_len, fds, max_fds, read_len, fd_num);
}

zx_status_t zx_unix_sock_write(zx_handle_t handle, const char* buf, int buf_len, int* fds,
                               int fd_num, uint32_t* written_len) {
  std::shared_ptr<UnixSocketChannelDispatcher> chan;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_WRITE, &chan);
  if (status != ZX_OK)
    return status;

  return chan->Write(buf, buf_len, fds, fd_num, written_len);
}
