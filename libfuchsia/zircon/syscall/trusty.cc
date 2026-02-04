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

#include <sys/types.h>
#include <trusty_ipc.h>
#include <zircon/syscalls.h>
#include <zircon/types.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <object/common.h>
#include <object/handle.h>
#include <object/tipc_chan_dispatcher.h>
#include <object/tipc_port_dispatcher.h>
#include <trusty/tipc.h>

static constexpr char kIpcDevicePath[] = "/dev/trusty-ipc-dev0";
static constexpr char kRpcDevicePath[] = "/dev/nbl-rpc0";

zx_status_t zx_tipc_port_create(const char* name, size_t name_len, uint32_t flags,
                                zx_handle_t* out) {
  std::shared_ptr<TipcPortDispatcher> port;
  zx_status_t status = TipcPortDispatcher::Create(0u, &port, /*rights=*/nullptr);
  if (status != ZX_OK)
    return status;

  status = port->Bind(/*uuid=*/nullptr, name, flags);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(port));
  return ZX_OK;
}

zx_status_t zx_tipc_port_accept(zx_handle_t handle_value, uuid_t* _uuid, size_t uuid_len,
                                zx_handle_t* out) {
  std::shared_ptr<TipcPortDispatcher> port;
  zx_status_t status = GetDispatcherWithRights(handle_value, ZX_RIGHT_WRITE, &port);
  if (status != ZX_OK)
    return status;

  std::shared_ptr<TipcChanDispatcher> chan;
  status = port->Accept(_uuid, uuid_len, &chan, /*rights=*/nullptr);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(chan));
  return ZX_OK;
}

static zx_status_t __connect_sock(std::string path, zx_handle_t* out) {
  std::shared_ptr<TipcPortDispatcher> port;
  zx_status_t status = GlobalTipcPortRegistry::Find(path, &port);
  if (status != ZX_OK)
    return status;

  std::shared_ptr<TipcChanDispatcher> chan;
  status = port->Connect(path, &chan);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(chan));
  return ZX_OK;
}

static zx_status_t __connect_fd(nebula::base::unique_fd fd, ChanBackend backend,
                                zx_handle_t* out) {
  if (!fd.ok())
    return ZX_ERR_INVALID_ARGS;

  std::shared_ptr<TipcChanDispatcher> chan;
  zx_status_t status = TipcChanDispatcher::CreateFromFileDescriptor(std::move(fd), backend, &chan,
                                                                    /*rights=*/nullptr);
  if (status != ZX_OK)
    return status;

  *out = Handle::ToU64(Handle::Make(chan));
  return ZX_OK;
}

zx_status_t zx_tipc_port_connect(const char* name, size_t name_len, uint32_t flags,
                                 zx_handle_t* out) {
  if (flags == IPC_CONNECT_UNIX_SOCK)
    return __connect_sock(name, out);

  if (flags == IPC_CONNECT_IPC)
    return __connect_fd(nebula::base::unique_fd(tipc_connect(kIpcDevicePath, name)),
                        ChanBackend::kIpc, out);

  if (flags == IPC_CONNECT_RPC)
    return __connect_fd(nebula::base::unique_fd(open(kRpcDevicePath, O_RDWR)), ChanBackend::kRpc,
                        out);

  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t zx_tipc_chan_read(zx_handle_t handle, uint32_t options, void* bytes,
                              zx_handle_t* handles, uint32_t num_bytes, uint32_t num_handles,
                              uint32_t* actual_bytes, uint32_t* actual_handles) {
  std::shared_ptr<TipcChanDispatcher> chan;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_READ, &chan);
  if (status != ZX_OK)
    return status;

  return chan->Read(bytes, handles, num_bytes, num_handles, actual_bytes, actual_handles);
}

zx_status_t zx_tipc_chan_write(zx_handle_t handle, uint32_t options, const void* bytes,
                               uint32_t num_bytes, const zx_handle_t* handles,
                               uint32_t num_handles) {
  std::shared_ptr<TipcChanDispatcher> chan;
  zx_status_t status = GetDispatcherWithRights(handle, ZX_RIGHT_READ, &chan);
  if (status != ZX_OK)
    return status;

  return chan->Write(bytes, num_bytes, handles, num_handles);
}