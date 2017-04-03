// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_config.h"

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace opera {

TEST(SyncConfigTest, DefaultSyncURL) {
  EXPECT_EQ(GURL("https://sync.opera.com/api/sync"),
            SyncConfig::SyncServerURL());
}

TEST(SyncConfigTest, DefaultAuthURL) {
  EXPECT_EQ(GURL("https://auth.opera.com/"),
            SyncConfig::AuthServerURL());
}

TEST(SyncConfigTest, SyncURLFromCommandLine) {
  const char kMockUrl[] = { "http://127.0.0.1:5222" };
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kSyncServiceURL, kMockUrl);

  EXPECT_EQ(GURL(kMockUrl), SyncConfig::SyncServerURL(command_line));
}

TEST(SyncConfigTest, AuthURLFromCommandLine) {
  const char kMockUrl[] = { "http://127.0.0.1:5222" };
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(kSyncAuthURLSwitch, kMockUrl);

  EXPECT_EQ(GURL(kMockUrl), SyncConfig::AuthServerURL(command_line));
}
}  // namespace opera
