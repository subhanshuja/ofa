// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Modified by Opera Software ASA
// @copied-from chromium/src/sync/util/get_session_name.h
// @final-synchronized 007879930a4b673bab147ee4f61fbcee6db58ede

#ifndef COMMON_OAUTH2_DEVICE_NAME_GET_SESSION_NAME_H_
#define COMMON_OAUTH2_DEVICE_NAME_GET_SESSION_NAME_H_

#include <string>

namespace opera {
namespace oauth2 {
namespace session_name {
std::string GetSessionNameImpl();
}  // namespace session_name
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_DEVICE_NAME_GET_SESSION_NAME_H_
