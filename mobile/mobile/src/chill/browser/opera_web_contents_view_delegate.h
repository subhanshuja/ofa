// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_OPERA_WEB_CONTENTS_VIEW_DELEGATE_H_
#define CHILL_BROWSER_OPERA_WEB_CONTENTS_VIEW_DELEGATE_H_

#include "content/public/browser/web_contents_view_delegate.h"

namespace content {
class WebContents;
class WebDragDestDelegate;
}

namespace opera {

content::WebContentsViewDelegate* CreateOperaWebContentsViewDelegate(
    content::WebContents* web_contents);

class OperaWebContentsViewDelegate : public content::WebContentsViewDelegate {
 public:
  explicit OperaWebContentsViewDelegate(content::WebContents* web_contents)
      : web_contents_(web_contents) {
  }

  // Shows a context menu.
  virtual void ShowContextMenu(content::RenderFrameHost* render_frame_host,
      const content::ContextMenuParams& params) override;

 private:
  content::WebContents* web_contents_;  // not owned
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_WEB_CONTENTS_VIEW_DELEGATE_H_
