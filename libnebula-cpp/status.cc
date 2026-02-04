
/*
 * Copyright (C) 2021 GoldenRiver Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lib/nebula/base/status.h"

#include <cstring>
#include <limits>

namespace nebula {
namespace base {

const char* StatusToString(Status status) {
  switch (status) {
    case Status::kOk:
      return "Ok";
    case Status::kErrGeneric:
      return "ErrGeneric";
    case Status::kErrNotFound:
      return "ErrNotFound";
    case Status::kErrNotReady:
      return "ErrNotReady";
    case Status::kErrNoMsg:
      return "ErrNoMsg";
    case Status::kErrNoMemory:
      return "ErrNoMemory";
    case Status::kErrAlreadyStarted:
      return "ErrAlreadyStarted";
    case Status::kErrNotValid:
      return "ErrNotValid";
    case Status::kErrInvalidArgs:
      return "ErrInvalidArgs";
    case Status::kErrNotEnoughBuffer:
      return "ErrNotEnoughBuffer";
    case Status::kErrNotSuspended:
      return "ErrNotSuspended";
    case Status::kErrObjectDestroyed:
      return "ErrObjectDestroyed";
    case Status::kErrNotBlocked:
      return "ErrNotBlocked";
    case Status::kErrTimedOut:
      return "ErrTimedOut";
    case Status::kErrAlreadyExists:
      return "ErrAlreadyExists";
    case Status::kErrChannelClosed:
      return "ErrChannelClosed";
    case Status::kErrOffline:
      return "ErrOffline";
    case Status::kErrNotAllowed:
      return "ErrNotAllowed";
    case Status::kErrBadPath:
      return "ErrBadPath";
    case Status::kErrAlreadyMounted:
      return "ErrAlreadyMounted";
    case Status::kErrIo:
      return "ErrIo";
    case Status::kErrNotDir:
      return "ErrNotDir";
    case Status::kErrNotFile:
      return "ErrNotFile";
    case Status::kErrRecurseTooDeep:
      return "ErrRecurseTooDeep";
    case Status::kErrNotSupported:
      return "ErrNotSupported";
    case Status::kErrTooBig:
      return "ErrTooBig";
    case Status::kErrCancelled:
      return "ErrCancelled";
    case Status::kErrNotImplemented:
      return "ErrNotImplemented";
    case Status::kErrChecksumFail:
      return "ErrChecksumFail";
    case Status::kErrCrcFail:
      return "ErrCrcFail";
    case Status::kErrCmdUnknown:
      return "ErrCmdUnknown";
    case Status::kErrBadState:
      return "ErrBadState";
    case Status::kErrBadLen:
      return "ErrBadLen";
    case Status::kErrBusy:
      return "ErrBusy";
    case Status::kErrThreadDetached:
      return "ErrThreadDetached";
    case Status::kErrI2cNack:
      return "ErrI2cNack";
    case Status::kErrAlreadyExpired:
      return "ErrAlreadyExpired";
    case Status::kErrOutOfRange:
      return "ErrOutOfRange";
    case Status::kErrNotConfigured:
      return "ErrNotConfigured";
    case Status::kErrNotMounted:
      return "ErrNotMounted";
    case Status::kErrFault:
      return "ErrFault";
    case Status::kErrNoResources:
      return "ErrNoResources";
    case Status::kErrBadHandle:
      return "ErrBadHandle";
    case Status::kErrAccessDenied:
      return "ErrAccessDenied";
    case Status::kErrPartialWrite:
      return "ErrPartialWrite";
    case Status::kErrCreateHandle:
      return "ErrCreateHandle";
    case Status::kErrCreateChildHandle:
      return "ErrCreateChildHandle";
    case Status::kErrMmap:
      return "ErrMmap";
    case Status::kErrOutOfMem:
      return "ErrOutOfMem";
    case Status::kErrConflictArgs:
      return "ErrConflictArgs";
    case Status::kErrTaRunFailed:
      return "ErrTaRunFailed";
  }
  return "ErrUnknownStatus";
}

Status BytesToStatus(std::string bytes) {
  if (bytes.size() < sizeof(Status))
    return static_cast<Status>(std::numeric_limits<Status>::min());
  return static_cast<Status>(*((int32_t*)&bytes[0]));
}

std::string StatusToBytes(Status status) {
  std::string result;
  result.resize(sizeof(Status));
  std::memcpy(&result[0], &status, sizeof(Status));
  return result;
}

std::ostream& operator<<(std::ostream& stream, const Status& s) {
  stream << StatusToString(s);
  return stream;
}

}  // namespace base
}  // namespace nebula