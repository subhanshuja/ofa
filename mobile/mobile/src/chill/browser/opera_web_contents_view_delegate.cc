// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/opera_web_contents_view_delegate.h"

#include <string>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "content/public/browser/android/content_view_core.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/context_menu_params.h"
#include "ui/gfx/android/device_display_info.h"

#include "chill/browser/context_menu_helper.h"
#include "chill/browser/chromium_tab.h"

namespace opera {

content::WebContentsViewDelegate* CreateOperaWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return new OperaWebContentsViewDelegate(web_contents);
}

void OperaWebContentsViewDelegate::ShowContextMenu(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params) {

  ChromiumTab* tab = ChromiumTab::FromWebContents(web_contents_);
  bool is_webapp = tab && tab->IsWebApp();
  bool is_searchable_form_field = !params.keyword_url.is_empty();
  bool may_show_add_search_engine = is_searchable_form_field && !is_webapp;

  // Display paste pop-up only when selection is empty and editable. Also check
  // if we need a custom menu (to show add-search-engine menu item).
  if (params.is_editable && params.selection_text.empty() &&
      !may_show_add_search_engine) {
    content::ContentViewCore* content_view_core =
        content::ContentViewCore::FromWebContents(web_contents_);
    if (content_view_core) {
      content_view_core->ShowPastePopup(params.selection_start.x(),
                                        params.selection_start.y());
      return;
    }
  }

  gfx::DeviceDisplayInfo device_info;
  const float device_scale_factor = device_info.GetDIPScale();

  content::ContextMenuParams modified_params(params);
  modified_params.x *= device_scale_factor;
  modified_params.y *= device_scale_factor;

  ContextMenuHelper* helper = ContextMenuHelper::FromWebContents(web_contents_);
  if (helper)
    helper->ShowContextMenu(modified_params);
}

}  // namespace opera
