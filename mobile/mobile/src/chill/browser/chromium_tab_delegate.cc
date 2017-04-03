// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/chromium_tab_delegate.h"

#include "content/browser/frame_host/navigation_entry_impl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

#include "chill/browser/chromium_tab.h"

namespace gfx {
class Point;
}  // namespace gfx

namespace opera {

ChromiumTabDelegate* ChromiumTabDelegate::FromWebContents(
    const content::WebContents* web_contents) {
  ChromiumTab* tab = ChromiumTab::FromWebContents(web_contents);
  return tab != NULL ? tab->GetDelegate() : NULL;
}

/*static*/
ChromiumTabDelegate* ChromiumTabDelegate::FromID(int render_process_id,
                                                 int render_view_id) {
  content::RenderViewHost* render_view_host =
      content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (render_view_host) {
    content::WebContents* web_contents =
        content::WebContents::FromRenderViewHost(render_view_host);
    if (web_contents)
      return ChromiumTabDelegate::FromWebContents(web_contents);
  }
  return NULL;
}

}  // namespace opera

