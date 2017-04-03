// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/error_pages/error_page.h"
#include "common/error_pages/localized_error.h"

#include "base/values.h"
#include "content/public/renderer/render_thread.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"

#include "opera_common/grit/opera_common_resources.h"

void GenerateErrorPage(const blink::WebURLRequest& failed_request,
                       const blink::WebURLError& error,
                       std::string* error_html,
                       base::string16* error_description) {
  if (error_html) {
    base::DictionaryValue error_strings;
    LocalizedError::GetStrings(error, &error_strings,
                               content::RenderThread::Get()->GetLocale());

    const int resource_id = IDR_NET_ERROR_HTML;

    const base::StringPiece template_html(
        ResourceBundle::GetSharedInstance().GetRawDataResource(resource_id));
    if (template_html.empty()) {
      NOTREACHED() << "unable to load template. ID: " << resource_id;
    } else {
      // "t" is the id of the templates root node.
      *error_html = webui::GetTemplatesHtml(template_html, &error_strings, "t");
    }
  }

  if (error_description) {
    *error_description = LocalizedError::GetErrorDetails(error);
  }
}

