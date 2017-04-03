// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Modified by Opera Software ASA
// @copied-from chromium/src/sync/util/get_session_name.cc
// @final-synchronized 007879930a4b673bab147ee4f61fbcee6db58ede

#include "common/oauth2/device_name/get_session_name.h"

#include "base/strings/string_util.h"
#include "base/sys_info.h"

#if defined(OS_LINUX)
#include <limits.h>  // for HOST_NAME_MAX
#include <unistd.h>  // for gethostname()

#include "base/linux_util.h"
#elif defined(OS_MACOSX)

#include <stddef.h>
#include <sys/sysctl.h>
#import <SystemConfiguration/SCDynamicStoreCopySpecific.h>

#include "base/mac/scoped_cftyperef.h"
#include "base/strings/sys_string_conversions.h"
#elif defined(OS_WIN)
#include <windows.h>

#include "base/strings/utf_string_conversions.h"
#endif

namespace opera {
namespace oauth2 {
namespace session_name {

namespace {
std::string GetSessionNameInternal() {
#if defined(OS_LINUX)
  char hostname[HOST_NAME_MAX];
  if (gethostname(hostname, HOST_NAME_MAX) == 0)  // Success.
    return hostname;
  return base::GetLinuxDistro();
#elif defined(OS_MACOSX)
  // Do not use NSHost currentHost, as it's very slow. http://crbug.com/138570
  SCDynamicStoreContext context = {0, NULL, NULL, NULL};
  base::ScopedCFTypeRef<SCDynamicStoreRef> store(SCDynamicStoreCreate(
      kCFAllocatorDefault, CFSTR("chrome_sync"), NULL, &context));
  base::ScopedCFTypeRef<CFStringRef> machine_name(
      SCDynamicStoreCopyLocalHostName(store.get()));
  if (machine_name.get())
    return base::SysCFStringRefToUTF8(machine_name.get());

  // Fall back to get computer name.
  base::ScopedCFTypeRef<CFStringRef> computer_name(
      SCDynamicStoreCopyComputerName(store.get(), NULL));
  if (computer_name.get())
    return base::SysCFStringRefToUTF8(computer_name.get());

  // If all else fails, return to using a slightly nicer version of the
  // hardware model.
  char modelBuffer[256];
  size_t length = sizeof(modelBuffer);
  if (!sysctlbyname("hw.model", modelBuffer, &length, NULL, 0)) {
    for (size_t i = 0; i < length; i++) {
      if (base::IsAsciiDigit(modelBuffer[i]))
        return std::string(modelBuffer, 0, i);
    }
    return std::string(modelBuffer, 0, length);
  }
  return "Unknown";
#elif defined(OS_WIN)
  wchar_t computer_name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
  DWORD size = arraysize(computer_name);
  if (::GetComputerNameW(computer_name, &size)) {
    std::string result;
    bool conversion_successful = base::WideToUTF8(computer_name, size, &result);
    DCHECK(conversion_successful);
    return result;
  }
#else
  NOTIMPLEMENTED();
#endif
  return std::string();
}
}  // namespace

std::string GetSessionNameImpl() {
  std::string session_name = GetSessionNameInternal();

  if (session_name == "Unknown" || session_name.empty())
    session_name = base::SysInfo::OperatingSystemName();

  DCHECK(base::IsStringUTF8(session_name));
  return session_name;
}

}  // namespace session_name
}  // namespace oauth2
}  // namespace opera
