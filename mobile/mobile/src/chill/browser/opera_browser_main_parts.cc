// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_browser_main_parts.cc
// @final-synchronized

#include "chill/browser/opera_browser_main_parts.h"

#include "base/android/locale_utils.h"
#include "base/android/path_utils.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "cc/base/switches.h"
#include "components/crash/content/browser/crash_dump_manager_android.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "device/geolocation/geolocation_delegate.h"
#include "device/geolocation/geolocation_provider.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/network_change_notifier.h"
#include "net/url_request/url_request_context_getter.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "ui/base/ui_base_paths.h"
#include "url/gurl.h"

#include "common/breakpad/breakpad_reporter.h"
#include "common/sitepatcher/browser/op_site_patcher_config.h"
#include "common/sitepatcher/browser/op_site_patcher.h"
#include "common/sync/sync.h"

#include "mobile/common/favorites/favorites.h"

#include "chill/app/native_interface.h"
#include "chill/browser/about_flags.h"
#include "chill/browser/media_capture_devices_dispatcher.h"
#include "chill/browser/net/opera_net_log.h"
#include "chill/browser/opera_access_token_store.h"
#include "chill/browser/opera_browser_context.h"
#include "chill/browser/opera_browser_context_keyed_service_factories.h"
#include "chill/browser/opera_devtools_manager_delegate.h"
#include "chill/browser/opera_content_browser_client.h"
#include "chill/browser/push_messaging/opera_push_messaging_service.h"
#include "chill/browser/webui/opera_web_ui_controller_factory.h"
#include "chill/browser/sitepatcher_config.h"
#include "chill/common/cpu_util.h"
#include "chill/common/opera_switches.h"
#include "chill/common/paths.h"

namespace opera {

namespace {

// A provider of services for Geolocation.
class OperaGeolocationDelegate : public device::GeolocationDelegate {
 public:
  explicit OperaGeolocationDelegate(OperaBrowserContext* context)
      : context_(context) {}

  scoped_refptr<device::AccessTokenStore> CreateAccessTokenStore() final {
    return new OperaAccessTokenStore(context_);
  }

 private:
  OperaBrowserContext* context_;
  DISALLOW_COPY_AND_ASSIGN(OperaGeolocationDelegate);
};

OpSitepatcherConfig CreateSitePatcherConfig(base::FilePath app_base_path) {
  OpSitepatcherConfig config;

  config.update_check_url = BUILDFLAG(OPR_SITEPATCHER_UPDATE_CHECK_URL);
  config.browser_js_url = BUILDFLAG(OPR_SITEPATCHER_BROWSER_JS_URL);
  config.prefs_override_url = BUILDFLAG(OPR_SITEPATCHER_PREFS_OVERRIDE_URL);
  config.web_bluetooth_blacklist_url =
      BUILDFLAG(OPR_SITEPATCHER_WEB_BLUETOOTH_URL);

  config.verify_site_prefs = true;

  config.update_prefs_file =
      app_base_path.Append(FILE_PATH_LITERAL("update_prefs.json"));
  config.prefs_override_file =
      app_base_path.Append(FILE_PATH_LITERAL("prefs_override.json"));
  config.browser_js_file =
      app_base_path.Append(FILE_PATH_LITERAL("browser.js"));
  config.user_js_file = app_base_path.Append(FILE_PATH_LITERAL("user.js"));
  config.web_bluetooth_blacklist_file =
      app_base_path.Append(FILE_PATH_LITERAL("web_bluetooth_blacklist.json"));

  // Check once a day for production build and every minute for test build.
  config.update_check_interval =
      BUILDFLAG(OPR_SITEPATCHER_UPDATE_CHECK_INTERVAL_SEC);
  // Recheck on failure after an hour.
  config.update_fail_interval = 1 * 60 * 60;
  return config;
}

}  // namespace

OperaBrowserMainParts::OperaBrowserMainParts(
    const content::MainFunctionParams& parameters)
    : BrowserMainParts(),
      parameters_(parameters),
      run_message_loop_(true) {}

OperaBrowserMainParts::~OperaBrowserMainParts() {
}

void OperaBrowserMainParts::PreMainMessageLoopStart() {
  content::WebUIControllerFactory::RegisterFactory(
      OperaWebUIControllerFactory::GetInstance());
}

void OperaBrowserMainParts::PostMainMessageLoopStart() {
  base::MessageLoopForUI::current()->Start();
}

void OperaBrowserMainParts::PreEarlyInitialization() {
  net::NetworkChangeNotifier::SetFactory(
      new net::NetworkChangeNotifierFactoryAndroid());
}

void OperaBrowserMainParts::PreMainMessageLoopRun() {
  net_log_.reset(new OperaNetLog());
  browser_context_.reset(new OperaBrowserContext(false, net_log_.get()));
  off_the_record_browser_context_.reset(
      new OperaBrowserContext(true, net_log_.get()));

  device::GeolocationProvider::SetGeolocationDelegate(
      new OperaGeolocationDelegate(browser_context()));

  base::FilePath app_base_path;
  PathService::Get(base::DIR_ANDROID_APP_DATA, &app_base_path);

  OperaContentBrowserClient::Get()->InitPushMessagingStorage(app_base_path);
  OperaPushMessagingService::InitializeForContext(browser_context_.get());

  EnsureBrowserContextKeyedServiceFactoriesBuilt();

  // Force MediaCaptureDevicesDispatcher to be created on UI thread.
  MediaCaptureDevicesDispatcher::GetInstance();

  if (parameters_.ui_task) {
    parameters_.ui_task->Run();
    delete parameters_.ui_task;
    run_message_loop_ = false;
  }

  content::StoragePartition* partition =
      content::BrowserContext::GetStoragePartition(browser_context_.get(), NULL);

  site_patcher_ = new OpSitePatcher(partition->GetURLRequestContext(),
                                    CreateSitePatcherConfig(app_base_path));
  site_patcher_->Initialize();

  mobile::Favorites* favorites = mobile::Favorites::instance();

  mobile::Sync::GetInstance()->Initialize(
      app_base_path,
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI),
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::DB),
      browser_context_.get());

  favorites->SetBookmarksEnabled(true);
  favorites->SetSavedPagesEnabled(mobile::Favorites::DISABLED);

  favorites->SetModel(
      mobile::Sync::GetInstance()->GetBookmarkModel(),
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI),
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::FILE));

  // Kludge alert! app_base_path is /data/data/<pkg>/app_opera but
  // speed dials are stored in /data/data/<pkg>/files
  favorites->SetBaseDirectory(app_base_path.DirName().Append("files").value());

  OperaDevToolsManagerDelegate::StartHttpHandler(browser_context_.get());
}

bool OperaBrowserMainParts::MainMessageLoopRun(int* result_code) {
  return !run_message_loop_;
}

void OperaBrowserMainParts::PostMainMessageLoopRun() {
  browser_context_.reset();
  off_the_record_browser_context_.reset();
  OperaDevToolsManagerDelegate::StopHttpHandler();
}

int OperaBrowserMainParts::PreCreateThreads() {
  // Flag preferences must be set before any sub process is spawned since the
  // flags are passed as command-line switches.
  //
  // The flag configuration is loaded synchronously from the current thread,
  // which is safe since we're not yet in "multi-threaded mode".
  OperaContentBrowserClient::Get()->InitFlagPrefs();
  about_flags::ConvertFlagsToSwitches(
      OperaContentBrowserClient::Get()->flags_prefs(),
      base::CommandLine::ForCurrentProcess());

  // Only enable compressed tiles if the CPU supports the SIMD implementation
  // as the generic C++ code is too slow for real-time encoding.
  if (IsNeonSupported() && !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableTileCompression)) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    command_line->AppendSwitch(cc::switches::kEnableTileCompression);
  }

  // Enable the breakpad crash reporter.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableCrashReporter)) {
    base::FilePath crash_dumps_dir;
    if (PathService::Get(Paths::BREAKPAD_CRASH_DUMP, &crash_dumps_dir))
      crash_dump_manager_.reset(
        new breakpad::CrashDumpManager(crash_dumps_dir));
  }

  ui::SetLocalePaksStoredInApk(false);

  const std::string locale = base::android::GetDefaultLocale();
  ui::ResourceBundle::InitSharedInstanceWithLocale(
    locale, nullptr, ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);

  base::FilePath resources_pack_path;
  PathService::Get(ui::DIR_RESOURCE_PAKS_ANDROID, &resources_pack_path);
  ui::LoadMainAndroidPackFile("assets/opera.pak", resources_pack_path);

  return 0;
}

}  // namespace opera
