// Copyright 2017 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <zircon/types.h>

#include <object/dispatcher.h>

#include "lib/nebula/base/unique_fd.h"

enum class ChanBackend {
  kNone = 0,
  kIpc,
  kRpc,
  kUnixSock,
};

class TipcChanDispatcher final : public PeeredDispatcher<TipcChanDispatcher> {
 public:
  static zx_status_t Create(std::shared_ptr<TipcChanDispatcher>* dispatcher0,
                            std::shared_ptr<TipcChanDispatcher>* dispatcher1, zx_rights_t* rights);

  static zx_status_t CreateFromFileDescriptor(nebula::base::unique_fd fd, ChanBackend backend,
                                              std::shared_ptr<TipcChanDispatcher>* dispatcher,
                                              zx_rights_t* rights);
  ~TipcChanDispatcher();

  zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_TIPC_CHAN; }
  bool has_state_tracker() const final { return true; }
  zx_signals_t allowed_user_signals() const final { return IPC_HANDLE_POLL_SEND_UNBLOCKED; }
  void on_zero_handles() final;

  zx_status_t Read(void* bytes, zx_handle_t* handles, uint32_t num_bytes, uint32_t num_handles,
                   uint32_t* actual_bytes, uint32_t* actual_handles);

  zx_status_t Write(const void* bytes, uint32_t num_bytes, const zx_handle_t* user_handles,
                    uint32_t num_handles) NO_THREAD_SAFETY_ANALYSIS;

  // Overrides |Dispatcher|.
  int get_fd() const final { return chanfd_.get(); }
  void on_epoll_event(uint32_t events) final;

 private:
  explicit TipcChanDispatcher(std::shared_ptr<PeerHolder<TipcChanDispatcher>> holder);

  void Init(nebula::base::unique_fd chanfd, std::shared_ptr<TipcChanDispatcher> peer,
            ChanBackend backend);
  void OnPeerZeroHandlesLocked() NO_THREAD_SAFETY_ANALYSIS;

  uint32_t options_;
  nebula::base::unique_fd chanfd_;
  ChanBackend backend_;
  std::shared_ptr<TipcChanDispatcher> peer_;
};
