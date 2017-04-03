// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/common/logcat_util.h"

#include <string>
#include <vector>

#include "base/process/launch.h"
#include "content/public/browser/browser_thread.h"

namespace opera {

void LogcatUtil::StartFetchData(size_t max_num_lines,
                                LogcatDataCallback callback) {
  callback_ = callback;
  max_num_lines_ = max_num_lines;

  // Fetch the data on the IO thread
  content::BrowserThread::PostTask(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&LogcatUtil::FetchData, base::Unretained(this)));
}

void LogcatUtil::FetchData() {
  std::vector<std::string> argv;
  argv.push_back("logcat");
  argv.push_back("-d");

  if (max_num_lines_ != 0) {
    // Limit the number of lines from logcat
    argv.push_back("-t");
    argv.push_back(std::to_string(max_num_lines_));
  }

  std::string output;
  if (!base::GetAppOutput(argv, &output)) {
    // This will be shown instead
    output = "Error: could not get output from logcat.";
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&LogcatUtil::EndFetchData, base::Unretained(this), output));
}

void LogcatUtil::EndFetchData(const std::string data) {
  // Create the arguments & run on UI thread
  OpLogcatArguments args;
  args.results = data;
  callback_.Run(args);
}

}  // namespace opera
