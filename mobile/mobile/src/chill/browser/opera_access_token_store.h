// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/src/content/shell/geolocation/shell_access_token_store.h
// @final-synchronized

#ifndef CHILL_BROWSER_OPERA_ACCESS_TOKEN_STORE_H_
#define CHILL_BROWSER_OPERA_ACCESS_TOKEN_STORE_H_

#include "device/geolocation/access_token_store.h"

namespace opera {
class OperaBrowserContext;

// Dummy access token store used to initialise the network location provider.
class OperaAccessTokenStore : public device::AccessTokenStore {
 public:
  explicit OperaAccessTokenStore(OperaBrowserContext* opera_browser_context);

 private:
  ~OperaAccessTokenStore() override;

  void GetRequestContextOnUIThread(OperaBrowserContext* opera_browser_context);
  void RespondOnOriginatingThread(const LoadAccessTokensCallback& callback);

  // AccessTokenStore
  void LoadAccessTokens(const LoadAccessTokensCallback& callback) override;

  void SaveAccessToken(const GURL& server_url,
                       const base::string16& access_token) override;

  OperaBrowserContext* opera_browser_context_;
  net::URLRequestContextGetter* system_request_context_;

  DISALLOW_COPY_AND_ASSIGN(OperaAccessTokenStore);
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_ACCESS_TOKEN_STORE_H_
