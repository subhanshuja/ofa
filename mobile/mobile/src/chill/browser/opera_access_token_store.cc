// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/src/content/shell/geolocation/shell_access_token_store.cc
// @final-synchronized

#include "chill/browser/opera_access_token_store.h"

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"

#include "chill/browser/opera_browser_context.h"

namespace opera {

OperaAccessTokenStore::OperaAccessTokenStore(
    OperaBrowserContext* opera_browser_context)
    : opera_browser_context_(opera_browser_context),
      system_request_context_(NULL) {
}

OperaAccessTokenStore::~OperaAccessTokenStore() {
}

void OperaAccessTokenStore::LoadAccessTokens(
    const LoadAccessTokensCallback& callback) {
  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&OperaAccessTokenStore::GetRequestContextOnUIThread,
                 this,
                 opera_browser_context_),
      base::Bind(&OperaAccessTokenStore::RespondOnOriginatingThread,
                 this,
                 callback));
}

void OperaAccessTokenStore::GetRequestContextOnUIThread(
    OperaBrowserContext* opera_browser_context) {
  system_request_context_ =
      content::BrowserContext::GetDefaultStoragePartition(opera_browser_context)->
          GetURLRequestContext();
}

void OperaAccessTokenStore::RespondOnOriginatingThread(
    const LoadAccessTokensCallback& callback) {
  // Since content_shell is a test executable, rather than an end user program,
  // we provide a dummy access_token set to avoid hitting the server.
  AccessTokenMap access_token_map;
  access_token_map[GURL()] = base::ASCIIToUTF16("chromium_content_shell");
  callback.Run(access_token_map, system_request_context_);
  system_request_context_ = NULL;
}

void OperaAccessTokenStore::SaveAccessToken(
    const GURL& server_url, const base::string16& access_token) {
}

}  // namespace opera
