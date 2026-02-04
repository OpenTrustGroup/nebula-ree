// Copyright 2017 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <trusty_uuid.h>
#include <zircon/types.h>

#include <queue>
#include <unordered_map>

#include <object/dispatcher.h>
#include <object/tipc_chan_dispatcher.h>

#include "lib/nebula/base/unique_fd.h"

class TipcPortDispatcher;

class GlobalTipcPortRegistry {
 public:
  static zx_status_t Find(std::string path, std::shared_ptr<TipcPortDispatcher>* out);
  static zx_status_t Register(std::string path, std::shared_ptr<TipcPortDispatcher> port);
  static void Unregister(std::string path);
};

class TipcPortDispatcher final : public SoloDispatcher {
 public:
  static zx_status_t Create(uint32_t options, std::shared_ptr<TipcPortDispatcher>* dispatcher,
                            zx_rights_t* rights);

  explicit TipcPortDispatcher(uint32_t options);
  ~TipcPortDispatcher();

  zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_TIPC_PORT; }
  bool has_state_tracker() const final { return true; }
  void on_zero_handles() final;

  zx_status_t Bind(const uuid_t* uuid, const char* path, uint32_t flags);

  zx_status_t Accept(void* _uuid, size_t uuid_len, std::shared_ptr<TipcChanDispatcher>* out_disp,
                     zx_rights_t* rights);

  zx_status_t Connect(std::string path, std::shared_ptr<TipcChanDispatcher>* out);

 private:
  std::queue<std::shared_ptr<TipcChanDispatcher>> pending_connections_ GUARDED_BY(get_lock());

  uint32_t options_;
  std::string path_;
};
