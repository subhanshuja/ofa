// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_content_client.h
// @final-synchronized

#ifndef CHILL_COMMON_OPERA_CONTENT_CLIENT_H_
#define CHILL_COMMON_OPERA_CONTENT_CLIENT_H_

#include <string>

#include "base/compiler_specific.h"
#include "content/public/common/content_client.h"

namespace opera {

std::string GetUserAgent();
std::string GetProduct();

class OperaContentClient : public content::ContentClient {
 public:
  virtual ~OperaContentClient();

  std::string GetProduct() const override;
  std::string GetUserAgent() const override;
  std::string GetUserAgentOverride() const override;
  base::string16 GetLocalizedString(int message_id) const override;
  virtual base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  virtual base::RefCountedMemory* GetDataResourceBytes(
      int resource_id) const override;
};

}  // namespace opera

#endif  // CHILL_COMMON_OPERA_CONTENT_CLIENT_H_
