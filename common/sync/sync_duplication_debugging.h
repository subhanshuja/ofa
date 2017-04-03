// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_DUPLICATION_DEBUGGING_H_
#define COMMON_SYNC_SYNC_DUPLICATION_DEBUGGING_H_

#include <cstdio>
#include <fstream>
#include <string>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "chrome/common/chrome_paths.h"

namespace opera {

// NOTE this is temporary and made to be easily callable from everywhere
// (hence no .cc to simplify linking). Yes, it's fugly.
class SyncDuplicationDebugging {
 public:
  static inline bool LoggingEnabled() {
    return GetInstance()->logging_enabled_;
  }

  template <typename STR>
  inline void Log(const STR& msg, const char* file, int line) {
    if (SyncDuplicationDebugging::LoggingEnabled() &&
        !log_file_path_.empty()) {
      std::ofstream log_file(log_file_path_.c_str(), std::ofstream::app);
      if (log_file.good()) {
        const base::FilePath source_file = base::FilePath::FromUTF8Unsafe(file);
        log_file << "[" << base::Time::Now() << "] "
                 << source_file.BaseName().AsUTF8Unsafe() << ":" << line
                 << ":  " << msg << std::endl;
      }
    }
  }

  static SyncDuplicationDebugging* GetInstance() {
    return base::Singleton<SyncDuplicationDebugging>::get();
  }

  inline void RemoveLogfile() {
    if (log_file_path_.empty())
      // This happens if we've launched with !LoggingEnabled(). We need to be
      // able to clear the log file even when logging is not enabled.
      EstablishLogFilePath();
    std::remove(log_file_path_.c_str());
  }

 private:
  friend struct base::DefaultSingletonTraits<SyncDuplicationDebugging>;
  SyncDuplicationDebugging() {
    logging_enabled_ =
        base::Environment::Create()->HasVar("OPERA_DUPLICATION_LOGGING") ||
        base::CommandLine::ForCurrentProcess()->HasSwitch(
            "opera-duplication-logging");
    if (!logging_enabled_)
      return;
    EstablishLogFilePath();
  }

  ~SyncDuplicationDebugging() = default;

  void EstablishLogFilePath() {
    base::FilePath user_data_dir;
    if (base::CommandLine::ForCurrentProcess()->HasSwitch("user-data-dir")) {
      user_data_dir =
          base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
              "user-data-dir");
    } else {
      bool result = PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
      if (!result) {
        // This should only happen in unittests.
        DCHECK(log_file_path_.empty());
        return;
      }
    }
    log_file_path_ =
        user_data_dir.AppendASCII("sync_duplication.log").AsUTF8Unsafe();
  }

  bool logging_enabled_;
  std::string log_file_path_;
};

}  // namespace opera

#define SYNC_DUPLICATION_LOG(msg)                                      \
  ::opera::SyncDuplicationDebugging::GetInstance()->Log(msg, __FILE__, \
                                                        __LINE__);

#endif  // COMMON_SYNC_SYNC_DUPLICATION_DEBUGGING_H_
