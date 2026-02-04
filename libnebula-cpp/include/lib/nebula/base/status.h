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

#pragma once

#include <cstdint>
#include <ostream>
#include <string>

namespace nebula {
namespace base {

enum Status : int32_t {
  kOk = 0,
  kErrGeneric = -1,
  kErrNotFound = -2,
  kErrNotReady = -3,
  kErrNoMsg = -4,
  kErrNoMemory = -5,
  kErrAlreadyStarted = -6,
  kErrNotValid = -7,
  kErrInvalidArgs = -8,
  kErrNotEnoughBuffer = -9,
  kErrNotSuspended = -10,
  kErrObjectDestroyed = -11,
  kErrNotBlocked = -12,
  kErrTimedOut = -13,
  kErrAlreadyExists = -14,
  kErrChannelClosed = -15,
  kErrOffline = -16,
  kErrNotAllowed = -17,
  kErrBadPath = -18,
  kErrAlreadyMounted = -19,
  kErrIo = -20,
  kErrNotDir = -21,
  kErrNotFile = -22,
  kErrRecurseTooDeep = -23,
  kErrNotSupported = -24,
  kErrTooBig = -25,
  kErrCancelled = -26,
  kErrNotImplemented = -27,
  kErrChecksumFail = -28,
  kErrCrcFail = -29,
  kErrCmdUnknown = -30,
  kErrBadState = -31,
  kErrBadLen = -32,
  kErrBusy = -33,
  kErrThreadDetached = -34,
  kErrI2cNack = -35,
  kErrAlreadyExpired = -36,
  kErrOutOfRange = -37,
  kErrNotConfigured = -38,
  kErrNotMounted = -39,
  kErrFault = -40,
  kErrNoResources = -41,
  kErrBadHandle = -42,
  kErrAccessDenied = -43,
  kErrPartialWrite = -44,
  kErrCreateHandle = -1001,
  kErrCreateChildHandle = -1002,
  kErrMmap = -1003,
  kErrOutOfMem = -1004,
  kErrConflictArgs = -1005,
  kErrTaRunFailed = -1006,
};

const char* StatusToString(Status status);

Status BytesToStatus(std::string bytes);
std::string StatusToBytes(Status status);

std::ostream& operator<<(std::ostream& stream, const Status& s);

}  // namespace base
}  // namespace nebula

#if 0
// enum
grep '#define' libnbl_fwk_ree/include/nbl_fwk_ree/err.h |
grep -v NO_ERROR |
grep -v _ERR_H_ |
sed 's/.*NBL_ERR_/ERR_/g' | tr [A-Z] [a-z] | sed -r 's/(^|_)(\w)/\U\2/g' | sed 's/(//g' | sed 's/)//g' | awk '{print "k"$1"="$2","}'
// get_string()
grep '#define' libnbl_fwk_ree/include/nbl_fwk_ree/err.h |
grep -v NO_ERROR |
grep -v _ERR_H_ |
sed 's/.*NBL_ERR_/ERR_/g' |
sed 's/\s.*//g' | tr [A-Z] [a-z] | sed -r 's/(^|_)(\w)/\U\2/g' | awk '{print "case NblStatus::k"$1": return \""$1"\";";}'
#endif