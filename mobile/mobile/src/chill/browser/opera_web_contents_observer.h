// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_OPERA_WEB_CONTENTS_OBSERVER_H_
#define CHILL_BROWSER_OPERA_WEB_CONTENTS_OBSERVER_H_

#include <map>

#include "base/callback.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}  // namespace content

namespace opera {

class OperaWebContentsObserver
    : public content::WebContentsObserver {
 public:
  explicit OperaWebContentsObserver(content::WebContents* web_contents);
  virtual ~OperaWebContentsObserver();

  void UpdatePlayState(blink::WebAudioElementPlayState play_state) override;
  void DidAttachInterstitialPage() override;

 private:
  content::WebContents* web_contents_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_WEB_CONTENTS_OBSERVER_H_
