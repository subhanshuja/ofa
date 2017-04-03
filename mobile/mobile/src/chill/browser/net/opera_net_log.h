// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_net_log.h
// @final-synchronized

#ifndef CHILL_BROWSER_NET_OPERA_NET_LOG_H_
#define CHILL_BROWSER_NET_OPERA_NET_LOG_H_

#include "net/log/net_log.h"

namespace base {
class Value;
}  // namespace base

namespace net {
class WriteToFileNetLogObserver;
}

namespace opera {

// OperaNetLog is an implementation of NetLog that adds file loggers
// as its observers.
class OperaNetLog : public net::NetLog {
 public:
  OperaNetLog();
  virtual ~OperaNetLog();

  static base::Value* GetOperaConstants();

 private:
  std::unique_ptr<net::WriteToFileNetLogObserver> net_log_logger_;

  DISALLOW_COPY_AND_ASSIGN(OperaNetLog);
};

}  // namespace opera

#endif  // CHILL_BROWSER_NET_OPERA_NET_LOG_H_
