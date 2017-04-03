// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/webui/cobrand_settings_ui.h"

#include "base/bind.h"
#include "base/values.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "grit/wam_resources.h"

#include "chill/common/opera_url_constants.h"

#include "chill/browser/cobranding/obml_request.h"

namespace {

content::WebUIDataSource* CreateCobrandSettingsUIDataSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(opera::kOperaCobrandSettingsHost);

  source->AddResourcePath("cobrand_settings.js", IDR_COBRAND_SETTINGS_JS);
  source->SetDefaultResource(IDR_COBRAND_SETTINGS_HTML);

  return source;
}

class MessageHandler : public content::WebUIMessageHandler {
 public:
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "makeRequest",
        base::Bind(&MessageHandler::HandleRequest, base::Unretained(this)));
  }

 private:
  void HandleRequest(const base::ListValue* args) {
    std::string request_url;
    bool got_request_url = args->GetString(0, &request_url);
    DCHECK(got_request_url);

    opera::MakeBlindOBMLRequest(request_url);
  }
};

}  // namespace

namespace opera {

CobrandSettingsUI::CobrandSettingsUI(content::WebUI* web_ui,
                                     std::string source_name)
    : WebUIController(web_ui) {
  web_ui->AddMessageHandler(new MessageHandler());
  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                CreateCobrandSettingsUIDataSource());
}

}  // namespace opera
