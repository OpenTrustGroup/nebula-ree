// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <zircon/types.h>

#include <cstring>
#include <string>
#include <type_traits>

#include "lib/nebula/base/logging.h"
#include "lib/nebula/rpc/proto.h"

namespace nebula {
namespace rpc {

class RpcMessage {
 public:
  RpcMessage() { bytes_.resize(sizeof(rpc_message_header_t), 0); }
  explicit RpcMessage(std::string bytes) : bytes_(std::move(bytes)) {}

  ~RpcMessage() = default;

  RpcMessage(const RpcMessage&) = delete;
  RpcMessage& operator=(const RpcMessage&) = delete;

  RpcMessage(RpcMessage&&) = default;
  RpcMessage& operator=(RpcMessage&&) = default;

  bool has_header() const { return bytes_.size() >= sizeof(rpc_message_header_t); }

  const rpc_message_header_t& header() const {
    return *reinterpret_cast<const rpc_message_header_t*>(&bytes_[0]);
  }

  rpc_message_header_t& header() { return *reinterpret_cast<rpc_message_header_t*>(&bytes_[0]); }

  size_t header_size() const { return sizeof(rpc_message_header_t); }

  uint32_t flags() const { return header().flags; }
  void set_flags(uint32_t flags) { header().flags = flags; }

  uint32_t ordinal() const { return header().ordinal; }
  void set_ordinal(uint32_t ordinal) { header().ordinal = ordinal; }

  bool is_ack() { return header().flags & RPC_FLAG_ACK_BIT; }
  bool is_last() { return header().flags & RPC_FLAG_STOP_BIT; }

  zx_txid_t txid() const { return header().txid; }
  void set_txid(zx_txid_t txid) { header().txid = txid; }

  size_t size() const { return bytes_.size(); }
  size_t payload_size() const { return bytes_.size() - header_size(); }

  const char* payload_ptr() const { return &bytes_[0] + header_size(); }
  char* payload_ptr() { return &bytes_[0] + header_size(); }

  std::string payload() { return std::string(payload_ptr(), payload_size()); }

  const std::string& bytes() const { return bytes_; }
  std::string& bytes() { return bytes_; }

  template <typename T>
  T* GetPayloadAs() const {
    return reinterpret_cast<T*>(&bytes_[0] + header_size());
  }

  template <typename T>
  T* GetPtr(size_t offset) {
    return reinterpret_cast<T*>((void*)(&bytes_[0] + offset));
  }

  // Append |size| uninitialized data to payload and return its beginning offset
  size_t Alloc(size_t size) {
    size_t offset = bytes_.size();
    size_t new_size = offset + size;
    CHECK_GE(new_size, offset);
    bytes_.resize(new_size);
    return offset;
  }

  RpcMessage& Encode(const void* bytes, size_t num_bytes) {
    size_t offset = Alloc(num_bytes);
    char* out = GetPtr<char>(offset);
    std::memcpy(out, bytes, num_bytes);
    return *this;
  }

  RpcMessage& Encode(const std::string& buffer) { return Encode(buffer.data(), buffer.size()); }

  template <typename T>
  RpcMessage& Encode(const T val) {
    static_assert(std::is_integral<T>::value, "Integral required");
    return Encode(&val, sizeof(T));
  }

  void Finish() { header().flags |= RPC_FLAG_STOP_BIT; }

  bool operator==(const RpcMessage& other) const {
    return txid() == other.txid() && ordinal() == other.ordinal() &&
           payload_size() == other.payload_size() &&
           std::memcmp(payload_ptr(), other.payload_ptr(), payload_size()) == 0;
  }

  static constexpr uint32_t kTxidNoReply = 0;

 private:
  std::string bytes_;
};

static inline std::ostream& operator<<(std::ostream& os, const RpcMessage& message) {
  return os << "txid " << message.txid() << " ordinal " << message.ordinal() << " flags "
            << message.flags() << " payload_size " << message.payload_size();
}

}  // namespace rpc
}  // namespace nebula