// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_WEBUI_OPERA_WEB_UI_CONTROLLER_FACTORY_H_
#define CHILL_BROWSER_WEBUI_OPERA_WEB_UI_CONTROLLER_FACTORY_H_

#include "content/public/browser/web_ui_controller_factory.h"

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"
#include "content/public/browser/web_ui.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace opera {

class OperaWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) const override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) const override;
  bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                              const GURL& url) const override;
  content::WebUIController* CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) const override;

  static OperaWebUIControllerFactory* GetInstance();

 protected:
  OperaWebUIControllerFactory() {}
  virtual ~OperaWebUIControllerFactory() {}

 private:
  friend struct base::DefaultSingletonTraits<OperaWebUIControllerFactory>;

  DISALLOW_COPY_AND_ASSIGN(OperaWebUIControllerFactory);
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBUI_OPERA_WEB_UI_CONTROLLER_FACTORY_H_
