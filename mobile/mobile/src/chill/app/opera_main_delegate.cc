// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_main_delegate.cc
// @final-synchronized

#include "chill/app/opera_main_delegate.h"

#include "base/android/apk_assets.h"
#include "base/android/locale_utils.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/cpu.h"
#include "base/i18n/rtl.h"
#include "base/lazy_instance.h"
#include "base/posix/global_descriptors.h"
#include "components/crash/content/app/breakpad_linux.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "net/base/net_module.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

#include "ipc/ipc_message.h"  // For IPC_MESSAGE_LOG_ENABLED.

#if defined(IPC_MESSAGE_LOG_ENABLED)
#define IPC_MESSAGE_MACROS_LOG_ENABLED
#include "content/public/common/content_ipc_logging.h"
#define IPC_LOG_TABLE_ADD_ENTRY(msg_id, logger) \
    content::RegisterIPCLogger(msg_id, logger)
#include "chill/common/opera_messages.h"
#endif

#include "chill/app/native_interface.h"
#include "chill/app/opera_breakpad_client.h"
#include "chill/browser/opera_content_browser_client.h"
#include "chill/common/opera_descriptors.h"
#include "chill/common/opera_switches.h"
#include "chill/renderer/opera_content_renderer_client.h"

#include "common/breakpad/breakpad_reporter.h"
#include "common/java_bridge/java_bridge.h"
#include "common/net/net_resource_provider.h"
#include "common/settings/settings_manager.h"
#include "common/settings/settings_manager_delegate.h"

namespace opera {

namespace {

base::LazyInstance<OperaCrashReporterClient>::Leaky
    g_opera_breakpad_client = LAZY_INSTANCE_INITIALIZER;

class JavaBridgeObserver : public JavaBridge::Observer {
 public:
  virtual ~JavaBridgeObserver() {}

  JavaBridgeObserver() {
    JavaBridge::AddObserver(this, true);
  }

  void OnInterfaceSet() {
    SettingsManager::SetDelegate(
        NativeInterface::Get()->GetSettingsManagerDelegate());
    JavaBridge::RemoveObserver(this);

    delete this;
  }
};

}  // namespace

OperaMainDelegate::OperaMainDelegate() {
}

OperaMainDelegate::~OperaMainDelegate() {
}

bool OperaMainDelegate::BasicStartupComplete(int* exit_code) {
  // The content settings component requires one of these to be set
  ContentSettingsPattern::SetNonWildcardDomainNonPortScheme(
      "non-wildcard-domain-non-porth-scheme-placeholder");

  // Deleted in JavaBridgeObserver::OnInterfaceSet
  new JavaBridgeObserver();

  SetContentClient(&content_client_);
  return false;
}

void OperaMainDelegate::PreSandboxStartup() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);
  if (command_line->HasSwitch(switches::kEnableCrashReporter)) {
    crash_reporter::SetCrashReporterClient(g_opera_breakpad_client.Pointer());

    if (process_type != switches::kZygoteProcess) {
      if (process_type.empty()) {
        breakpad::InitCrashReporter(process_type);
      } else {
        breakpad::InitNonBrowserCrashReporterForAndroid(process_type);
      }
    }
  }

  base::i18n::SetICUDefaultLocale(base::android::GetDefaultLocale());

  if (process_type == switches::kRendererProcess) {
    InitializeResourceBundle();
  }
  // Configure modules that need access to resources.
  net::NetModule::SetResourceProvider(opera_common_net::NetResourceProvider);
#if defined(ARCH_CPU_ARM_FAMILY)
  // Create an instance of the CPU class to parse /proc/cpuinfo and cache
  // cpu_brand info.
  base::CPU cpu_info;
#endif
}

int OperaMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  if (!process_type.empty())
    return -1;

  // If no process type is specified, we are creating the main browser process.
  browser_runner_.reset(content::BrowserMainRunner::Create());
  int exit_code = browser_runner_->Initialize(main_function_params);
  DCHECK(exit_code < 0)
      << "BrowserRunner::Initialize failed in OperaMainDelegate";

  return exit_code;
}

void OperaMainDelegate::InitializeResourceBundle() {
  // On Android, the renderer runs with a different UID and can never access
  // the file system. Use the file descriptor passed in at launch time.
  auto global_descriptors = base::GlobalDescriptors::GetInstance();
  int pak_fd = global_descriptors->Get(kLocalePakDescriptor);
  base::MemoryMappedFile::Region pak_region =
      global_descriptors->GetRegion(kLocalePakDescriptor);
  ResourceBundle::InitSharedInstanceWithPakFileRegion(base::File(pak_fd),
                                                      pak_region);

  pak_fd = global_descriptors->Get(kOperaPakDescriptor);
  pak_region = global_descriptors->GetRegion(kOperaPakDescriptor);
  ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(
      base::File(pak_fd), pak_region, ui::SCALE_FACTOR_100P);
}

content::ContentBrowserClient* OperaMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new OperaContentBrowserClient);
  return browser_client_.get();
}

content::ContentRendererClient*
OperaMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new OperaContentRendererClient);
  return renderer_client_.get();
}

}  // namespace opera
