// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_MIGRATION_OBML_CONVERTER_WEB_UI_CONTROLLER_FACTORY_H_
#define CHILL_BROWSER_MIGRATION_OBML_CONVERTER_WEB_UI_CONTROLLER_FACTORY_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "url/gurl.h"

#include "chill/browser/migration/obml_font_info.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace opera {

// Specialized WebUIControllerFactory only permitting the creation of a single
// type of controller, for a single url, for a single intance of WebContents.
class OBMLConverterWebUIControllerFactory
    : public content::WebUIControllerFactory {
 public:
  OBMLConverterWebUIControllerFactory(
      GURL url,
      const content::WebContents* web_contents,
      const std::vector<OBMLFontInfo>* font_info,
      base::FilePath conversion_target_directory);

  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) const override;

  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) const override;
  bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                              const GURL& url) const override;

  content::WebUIController* CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) const override;

  // In lieu of a proper API for unregistering WebUIController factories, we can
  // at least disable the factory and not premit it to create new controllers.
  void Disable();

 protected:
  bool IsDisabled() const;

  virtual ~OBMLConverterWebUIControllerFactory() {}

 private:
  const GURL url_;
  const content::WebContents* web_contents_;
  const std::vector<OBMLFontInfo>* font_info_;
  base::FilePath conversion_target_directory_;

  DISALLOW_COPY_AND_ASSIGN(OBMLConverterWebUIControllerFactory);
};

}  // namespace opera

#endif  // CHILL_BROWSER_MIGRATION_OBML_CONVERTER_WEB_UI_CONTROLLER_FACTORY_H_
