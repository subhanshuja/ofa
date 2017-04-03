// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/webui/debug_zero_rating_ui.h"

#include "base/bind.h"
#include "base/values.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "grit/wam_resources.h"

#include "chill/browser/net/turbo_delegate.h"
#include "chill/browser/opera_browser_context.h"

namespace opera {

namespace {

TurboDelegate* turbo_delegate() {
  return OperaBrowserContext::GetDefaultBrowserContext()->turbo_delegate();
}

class DebugZeroRatingUIMessageHandler
    : public content::WebUIMessageHandler
    , private TurboDelegate::Observer {
 public:
  DebugZeroRatingUIMessageHandler();
  ~DebugZeroRatingUIMessageHandler();
  void RegisterMessages() override;

  void OnShouldUpdateRules(std::string tr_debug) override;
  void OnOperaTrDebug(std::string tr_debug) override;
  void OnTurboBypass(std::string bypass_warning) override;

  void FetchRules(const base::ListValue* args);

  void ToggleDebug(const base::ListValue* args);

  void ContentLoaded(const base::ListValue* args);

  void MCCMNCChanged(const base::ListValue* args);

 private:
  void LogMessage(const std::string& str);
  void LogWarning(const std::string& str);
  bool observing_;
};

DebugZeroRatingUIMessageHandler::DebugZeroRatingUIMessageHandler()
    : observing_(false) {
}

DebugZeroRatingUIMessageHandler::~DebugZeroRatingUIMessageHandler() {
    turbo_delegate()->RemoveObserver(this);
}

void DebugZeroRatingUIMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "fetchRules",
      base::Bind(&DebugZeroRatingUIMessageHandler::FetchRules,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "toggleDebug",
      base::Bind(&DebugZeroRatingUIMessageHandler::ToggleDebug,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "contentLoaded",
      base::Bind(&DebugZeroRatingUIMessageHandler::ContentLoaded,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "mccmncChanged",
      base::Bind(&DebugZeroRatingUIMessageHandler::MCCMNCChanged,
                 base::Unretained(this)));
}

void DebugZeroRatingUIMessageHandler::OnShouldUpdateRules(
    std::string update_message) {
  LogWarning(update_message);
}

void DebugZeroRatingUIMessageHandler::OnOperaTrDebug(std::string tr_debug) {
  LogMessage(tr_debug);
}

void DebugZeroRatingUIMessageHandler::OnTurboBypass(
    std::string bypass_warning) {
  LogWarning(bypass_warning);
}

void DebugZeroRatingUIMessageHandler::FetchRules(const base::ListValue* args) {
  if (turbo_delegate()) {
    turbo_delegate()->FetchZeroRatingRules();
  }
}

void DebugZeroRatingUIMessageHandler::ToggleDebug(const base::ListValue* args) {
  if (turbo_delegate()) {
    if (observing_) {
      turbo_delegate()->RemoveObserver(this);
    } else {
      turbo_delegate()->AddObserver(this);
    }

    observing_ = !observing_;

    web_ui()->CallJavascriptFunctionUnsafe("debug_zero_rating_ui.setDebugState",
                                           base::FundamentalValue(observing_));
  }
}

void DebugZeroRatingUIMessageHandler::ContentLoaded(
    const base::ListValue* args) {
#ifdef ENABLE_NETWORK_OVERRIDE
  web_ui()->CallJavascriptFunctionUnsafe("debug_zero_rating_ui.enableSpoofing");
#endif  // ENABLE_NETWORK_OVERRIDE
  CountryInformation info = turbo_delegate()->GetCountryInformation();
  web_ui()->CallJavascriptFunctionUnsafe(
      "debug_zero_rating_ui.setVisibleMCCMNC",
      base::StringValue(info.mobile_country_code),
      base::StringValue(info.mobile_network_code));
}

void DebugZeroRatingUIMessageHandler::MCCMNCChanged(
    const base::ListValue* args) {
#ifdef ENABLE_NETWORK_OVERRIDE
  DCHECK_EQ(2u, args->GetSize());

  std::string mcc;
  std::string mnc;

  if (!args->GetString(0, &mcc) || !args->GetString(1, &mnc))
    return;

  turbo_delegate()->SpoofMCCMNC(mcc, mnc);
#endif  // ENABLE_NETWORK_OVERRIDE
}

void DebugZeroRatingUIMessageHandler::LogMessage(const std::string& str) {
  web_ui()->CallJavascriptFunctionUnsafe("debug_zero_rating_ui.logMessage",
                                         base::StringValue(str));
}

void DebugZeroRatingUIMessageHandler::LogWarning(const std::string& str) {
  web_ui()->CallJavascriptFunctionUnsafe("debug_zero_rating_ui.logWarning",
                                         base::StringValue(str));
}

content::WebUIDataSource* CreateDataSource(const std::string& source_name) {
  content::WebUIDataSource* data_source =
      content::WebUIDataSource::Create(source_name);

  data_source->AddResourcePath("debug_zero_rating.js",
                               IDR_DEBUG_ZERO_RATING_JS);
  data_source->SetDefaultResource(IDR_DEBUG_ZERO_RATING_HTML);

  return data_source;
}

}  // namespace

DebugZeroRatingUI::DebugZeroRatingUI(content::WebUI* web_ui,
                                     std::string source_name)
    : WebUIController(web_ui) {
  web_ui->AddMessageHandler(new DebugZeroRatingUIMessageHandler());
  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                CreateDataSource(source_name));
}

}  // namespace opera
