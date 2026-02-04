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
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <trusty_ipc.h>

#include <object/tipc_port_dispatcher.h>

#include "lib/nebula/base/logging.h"
#include "lib/nebula/base/auto_lock.h"
#include "lib/nebula/base/mutex.h"

static nebula::base::Mutex g_ports_mu_;
static std::unordered_map<std::string, std::shared_ptr<TipcPortDispatcher>> g_ports_;

zx_status_t GlobalTipcPortRegistry::Find(std::string path,
                                         std::shared_ptr<TipcPortDispatcher>* out) {
  nebula::base::AutoLock lock(&g_ports_mu_);
  auto iter = g_ports_.find(path);
  if (iter == g_ports_.end())
    return ZX_ERR_NOT_FOUND;

  *out = iter->second;

  return ZX_OK;
}

zx_status_t GlobalTipcPortRegistry::Register(std::string path,
                                             std::shared_ptr<TipcPortDispatcher> port) {
  nebula::base::AutoLock lock(&g_ports_mu_);
  auto [_, success] = g_ports_.emplace(path, port);
  return success ? ZX_OK : ZX_ERR_ALREADY_BOUND;
}

void GlobalTipcPortRegistry::Unregister(std::string path) {
  nebula::base::AutoLock lock(&g_ports_mu_);
  g_ports_.erase(path);
}

// static
zx_status_t TipcPortDispatcher::Create(uint32_t options,
                                       std::shared_ptr<TipcPortDispatcher>* dispatcher,
                                       zx_rights_t* rights) {
  std::shared_ptr<TipcPortDispatcher> disp(new TipcPortDispatcher(options));
  if (!disp)
    return ZX_ERR_NO_MEMORY;

  *dispatcher = disp;
  return ZX_OK;
}

TipcPortDispatcher::TipcPortDispatcher(uint32_t options) : options_(options) {}

TipcPortDispatcher::~TipcPortDispatcher() {}

void TipcPortDispatcher::on_zero_handles() { GlobalTipcPortRegistry::Unregister(path_); }

zx_status_t TipcPortDispatcher::Bind(const uuid_t* uuid, const char* path, uint32_t flags) {
  zx_status_t status =
      GlobalTipcPortRegistry::Register(path, std::static_pointer_cast<TipcPortDispatcher>(self()));
  if (status != ZX_OK)
    return status;

  path_ = path;
  return ZX_OK;
}

zx_status_t TipcPortDispatcher::Accept(void* out_uuid, size_t uuid_len,
                                       std::shared_ptr<TipcChanDispatcher>* out_disp,
                                       zx_rights_t* rights) {
  nebula::base::AutoLock lock(get_lock());
  if (pending_connections_.empty())
    return ZX_ERR_NOT_FOUND;

  *out_disp = pending_connections_.front();
  pending_connections_.pop();

  if (pending_connections_.empty())
    UpdateStateLocked(IPC_HANDLE_POLL_READY, 0u);

  return ZX_OK;
}

zx_status_t TipcPortDispatcher::Connect(std::string path,
                                        std::shared_ptr<TipcChanDispatcher>* out) {
  CHECK_EQ(path, path_);

  std::shared_ptr<TipcChanDispatcher> ch0, ch1;
  zx_status_t status = TipcChanDispatcher::Create(&ch0, &ch1, nullptr);
  if (status != ZX_OK)
    return status;

  nebula::base::AutoLock lock(get_lock());
  *out = ch0;
  pending_connections_.push(ch1);

  UpdateStateLocked(0u, IPC_HANDLE_POLL_READY);

  return ZX_OK;
}