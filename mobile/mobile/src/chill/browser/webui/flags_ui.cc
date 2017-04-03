// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/webui/flags_ui.cc
// @last-synchronized 8b35b50891dce8d67afd00016f31e0aef50bdcd4

#include "chill/browser/webui/flags_ui.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ref_counted_memory.h"
#include "base/values.h"
#include "components/flags_ui/flags_ui_constants.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "grit/components_chromium_strings.h"
#include "grit/components_resources.h"
#include "grit/components_strings.h"
#include "grit/wam_resources.h"
#include "opera_common/grit/product_free_common_strings.h"
#include "opera_common/grit/product_related_common_strings.h"
#include "ui/base/l10n/l10n_util.h"

#include "chill/app/native_interface.h"
#include "chill/browser/about_flags.h"
#include "chill/browser/opera_content_browser_client.h"

namespace content {
class BrowserContext;
}

namespace opera {

namespace {

content::WebUIDataSource* CreateFlagsUIHTMLSource(const std::string& host) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(host);

  source->AddLocalizedString(flags_ui::kFlagsLongTitle,
                             IDS_FLAGS_UI_LONG_TITLE);
  source->AddLocalizedString(flags_ui::kFlagsTableTitle,
                             IDS_FLAGS_UI_TABLE_TITLE);
  source->AddLocalizedString(flags_ui::kFlagsWarningHeader,
                             IDS_FLAGS_UI_WARNING_HEADER);
  source->AddLocalizedString(flags_ui::kFlagsBlurb, IDS_FLAGS_UI_WARNING_TEXT);
  source->AddLocalizedString(flags_ui::kChannelPromoBeta,
                             IDS_FLAGS_UI_PROMOTE_BETA_CHANNEL);
  source->AddLocalizedString(flags_ui::kChannelPromoDev,
                             IDS_FLAGS_UI_PROMOTE_DEV_CHANNEL);
  source->AddLocalizedString(flags_ui::kFlagsUnsupportedTableTitle,
                             IDS_FLAGS_UI_UNSUPPORTED_TABLE_TITLE);
  source->AddLocalizedString(flags_ui::kFlagsNotSupported,
                             IDS_FLAGS_UI_NOT_AVAILABLE);
  source->AddLocalizedString(flags_ui::kFlagsRestartNotice,
                             IDS_FLAGS_UI_RELAUNCH_NOTICE_OPERA);
  source->AddLocalizedString(flags_ui::kFlagsRestartButton,
                             IDS_FLAGS_UI_RELAUNCH_BUTTON);
  source->AddLocalizedString(flags_ui::kResetAllButton,
                             IDS_FLAGS_UI_RESET_ALL_BUTTON);
  source->AddLocalizedString(flags_ui::kDisable, IDS_FLAGS_UI_DISABLE);
  source->AddLocalizedString(flags_ui::kEnable, IDS_FLAGS_UI_ENABLE);

  source->SetJsonPath("strings.js");
  source->AddResourcePath(flags_ui::kFlagsJS, IDR_FLAGS_JS);
  source->SetDefaultResource(IDR_FLAGS_HTML);
  return source;
}

////////////////////////////////////////////////////////////////////////////////
//
// FlagsDOMHandler
//
////////////////////////////////////////////////////////////////////////////////

// The handler for Javascript messages for the about:flags page.
class FlagsDOMHandler : public content::WebUIMessageHandler {
 public:
  FlagsDOMHandler() : access_(about_flags::kGeneralAccessFlagsOnly),
                      experimental_features_requested_(false) {
  }
  ~FlagsDOMHandler() override {}

  // Initializes the DOM handler with the provided flags storage and flags
  // access. If there were flags experiments requested from javascript before
  // this was called, it calls |HandleRequestFlagsExperiments| again.
  void Init(const scoped_refptr<FlagsJsonStorage>& flags_storage,
            about_flags::FlagAccess access);

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // Callback for the "requestExperimentFeatures" message.
  void HandleRequestExperimentalFeatures(const base::ListValue* args);

  // Callback for the "enableExperimentalFeature" message.
  void HandleEnableExperimentalFeatureMessage(const base::ListValue* args);

  // Callback for the "restartBrowser" message. Restores all tabs on restart.
  void HandleRestartBrowser(const base::ListValue* args);

  // Callback for the "resetAllFlags" message.
  void HandleResetAllFlags(const base::ListValue* args);

 private:
  scoped_refptr<FlagsJsonStorage> flags_storage_;
  about_flags::FlagAccess access_;
  bool experimental_features_requested_;

  DISALLOW_COPY_AND_ASSIGN(FlagsDOMHandler);
};

void FlagsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      flags_ui::kRequestExperimentalFeatures,
      base::Bind(&FlagsDOMHandler::HandleRequestExperimentalFeatures,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      flags_ui::kEnableExperimentalFeature,
      base::Bind(&FlagsDOMHandler::HandleEnableExperimentalFeatureMessage,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      flags_ui::kRestartBrowser,
      base::Bind(&FlagsDOMHandler::HandleRestartBrowser,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      flags_ui::kResetAllFlags,
      base::Bind(&FlagsDOMHandler::HandleResetAllFlags,
                 base::Unretained(this)));
}

void FlagsDOMHandler::Init(
    const scoped_refptr<FlagsJsonStorage>& flags_storage,
    about_flags::FlagAccess access) {
  flags_storage_ = flags_storage;
  access_ = access;

  if (experimental_features_requested_)
    HandleRequestExperimentalFeatures(NULL);
}

void FlagsDOMHandler::HandleRequestExperimentalFeatures(
    const base::ListValue* args) {
  experimental_features_requested_ = true;
  // Bail out if the handler hasn't been initialized yet. The request will be
  // handled after the initialization.
  if (!flags_storage_)
    return;

  base::DictionaryValue results;

  std::unique_ptr<base::ListValue> supported_features(new base::ListValue);
  std::unique_ptr<base::ListValue> unsupported_features(new base::ListValue);
  about_flags::GetFlagFeatureEntries(flags_storage_,
                                     access_,
                                     supported_features.get(),
                                     unsupported_features.get());
  results.Set(flags_ui::kSupportedFeatures, supported_features.release());
  results.Set(flags_ui::kUnsupportedFeatures, unsupported_features.release());
  results.SetBoolean(flags_ui::kNeedsRestart,
                     about_flags::IsRestartNeededToCommitChanges());
  results.SetBoolean(flags_ui::kShowOwnerWarning,
                     access_ == about_flags::kGeneralAccessFlagsOnly);
  results.SetBoolean(flags_ui::kShowBetaChannelPromotion, false);
  results.SetBoolean(flags_ui::kShowDevChannelPromotion, false);
  web_ui()->CallJavascriptFunctionUnsafe(flags_ui::kReturnExperimentalFeatures,
                                         results);
}

void FlagsDOMHandler::HandleEnableExperimentalFeatureMessage(
    const base::ListValue* args) {
  DCHECK(flags_storage_);
  DCHECK_EQ(2u, args->GetSize());
  if (args->GetSize() != 2)
    return;

  std::string entry_internal_name;
  std::string enable_str;
  if (!args->GetString(0, &entry_internal_name) ||
      !args->GetString(1, &enable_str))
    return;

  about_flags::SetFeatureEntryEnabled(flags_storage_, entry_internal_name,
                                      enable_str == "true");
}

void FlagsDOMHandler::HandleRestartBrowser(const base::ListValue* args) {
  DCHECK(flags_storage_);
  NativeInterface::Get()->RestartBrowser();
}

void FlagsDOMHandler::HandleResetAllFlags(const base::ListValue* args) {
  DCHECK(flags_storage_);
  about_flags::ResetAllFlags(flags_storage_);
}

}  // namespace

FlagsUI::FlagsUI(content::WebUI* web_ui, const std::string& host)
    : WebUIController(web_ui),
      weak_factory_(this) {
  FlagsDOMHandler* handler = new FlagsDOMHandler();
  web_ui->AddMessageHandler(handler);

  handler->Init(OperaContentBrowserClient::Get()->flags_prefs(),
                about_flags::kOwnerAccessToFlags);

  content::BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();

  // Set up the about:flags source.
  content::WebUIDataSource::Add(browser_context, CreateFlagsUIHTMLSource(host));
}

FlagsUI::~FlagsUI() {
}

}  // namespace opera
