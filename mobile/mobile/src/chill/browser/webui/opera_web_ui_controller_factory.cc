// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/webui/opera_web_ui_controller_factory.h"

#include <string>

#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#include "url/gurl.h"

#include "chill/app/tester_mode.h"
#include "chill/browser/webui/about_ui.h"
#include "chill/browser/webui/cobrand_settings_ui.h"
#include "chill/browser/webui/debug_zero_rating_ui.h"
#include "chill/browser/webui/flags_ui.h"
#include "chill/browser/webui/net_internals_ui.h"
#include "chill/browser/webui/version_ui.h"
#include "chill/browser/webui/logcat_ui.h"
#include "chill/common/opera_url_constants.h"

using content::WebUI;

namespace opera {

namespace {

typedef content::WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                          const GURL& url);

template<class T>
content::WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui, url.host());
}

WebUIFactoryFunction GetWebUIFactoryFunction(const GURL& url) {
  if (!url.SchemeIs(kOperaUIScheme)) {
    return NULL;
  }

  const std::string host = url.host();

  if (host == kOperaAboutHost)
    return &NewWebUI<AboutUI>;
  if (host == kOperaCobrandSettingsHost && TesterMode::Enabled())
    return &NewWebUI<CobrandSettingsUI>;
  if (host == kOperaFlagsHost)
    return &NewWebUI<FlagsUI>;
  if (host == kOperaVersionHost)
    return &NewWebUI<VersionUI>;
  if (host == kOperaNetInternalsHost)
    return NULL; // See WAM-6394
#ifdef TURBO2_PROXY_HOST_ZERO_RATE_SLOT
  if (host == kOperaDebugZeroRatingHost)
    return &NewWebUI<DebugZeroRatingUI>;
#endif  // TURBO2_PROXY_HOST_ZERO_RATE_SLOT
  if (host == kOperaLogcatHost)
    return &NewWebUI<LogcatUI>;

  return NULL;
}

}  // namespace

WebUI::TypeID OperaWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context, const GURL& url) const {
  WebUIFactoryFunction function = GetWebUIFactoryFunction(url);
  return function ? reinterpret_cast<WebUI::TypeID>(function) : WebUI::kNoWebUI;
}

bool OperaWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context, const GURL& url) const {
  return GetWebUIType(browser_context, url) != WebUI::kNoWebUI;
}

bool OperaWebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  const std::string host = url.host();

  return UseWebUIForURL(browser_context, url) &&
         (host == kOperaCobrandSettingsHost ||
#ifdef TURBO2_PROXY_HOST_ZERO_RATE_SLOT
          host == kOperaDebugZeroRatingHost ||
#endif  // TURBO2_PROXY_HOST_ZERO_RATE_SLOT
          host == kOperaFlagsHost);
}

content::WebUIController*
OperaWebUIControllerFactory::CreateWebUIControllerForURL(
    content::WebUI* web_ui,
    const GURL& url) const {
  WebUIFactoryFunction function = GetWebUIFactoryFunction(url);
  if (!function)
    return NULL;

  return (*function)(web_ui, url);
}

// static
OperaWebUIControllerFactory* OperaWebUIControllerFactory::GetInstance() {
  return base::Singleton< OperaWebUIControllerFactory,
      base::DefaultSingletonTraits<OperaWebUIControllerFactory> >::get();
}

}  // namespace opera
