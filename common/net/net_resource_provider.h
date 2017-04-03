// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/common/net/net_resource_provider.h
// @last-synchronized 3f6e8ceecb1c5386bd90e7ab53f60b1e83bcbfa4

#ifndef COMMON_NET_NET_RESOURCE_PROVIDER_H_
#define COMMON_NET_NET_RESOURCE_PROVIDER_H_

#include "base/strings/string_piece.h"

namespace opera_common_net {

// This is called indirectly by the network layer to access resources.
base::StringPiece NetResourceProvider(int key);

}  // namespace opera_common_net

#endif  // COMMON_NET_NET_RESOURCE_PROVIDER_H_
