// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Contains the definition of a few helper functions used for generating sync
// URLs.

#include "components/sync/engine_impl/net/url_translator.h"

#include "net/base/escape.h"

using std::string;

namespace syncer {

namespace {
// Parameters that the server understands. (here, a-Z)
const char kParameterClient[] = "client";
const char kParameterClientID[] = "client_id";

#if defined(GOOGLE_CHROME_BUILD)
const char kClientName[] = "Google Chrome";
#elif defined(OPERA_SYNC)
const char kClientName[] = "Opera";
#else
const char kClientName[] = "Chromium";
#endif  // defined(GOOGLE_CHROME_BUILD)
}  // namespace

// Convenience wrappers around CgiEscapePath().
string CgiEscapeString(const char* src) {
  return CgiEscapeString(string(src));
}

string CgiEscapeString(const string& src) {
#if defined(OPERA_SYNC)
  return net::EscapeQueryParamValue(src, true);
#else
  return net::EscapeUrlEncodedData(src, true);
#endif  // OPERA_SYNC
}

// This method appends the query string to the sync server path.
string MakeSyncServerPath(const string& path, const string& query_string) {
  string result = path;
  result.append("?");
  result.append(query_string);
  return result;
}

string MakeSyncQueryString(const string& client_id) {
  string query;
  query += kParameterClient;
  query += "=" + CgiEscapeString(kClientName);
  query += "&";
  query += kParameterClientID;
  query += "=" + CgiEscapeString(client_id);
  return query;
}

}  // namespace syncer
