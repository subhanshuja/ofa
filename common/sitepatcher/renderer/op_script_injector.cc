// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/renderer/op_script_injector.h"

#include "common/sitepatcher/renderer/op_script_store.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebDataSource.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "url/gurl.h"

namespace op_script_injector_utils {

namespace {
bool InjectAllowed(blink::WebLocalFrame* frame) {
  GURL url(frame->document().url());

  // When webkit creates iframe it first loads there about:blank regardless
  // of actual src value. We do not want to start browser.js in that case
  // as this is not "real" about:blank load. When src gets loaded frame
  // data source request will become valid for this iframe.
  if (url.SchemeIs("http") ||
      url.SchemeIs("https") ||
      (url == GURL("about:blank") &&
          frame->dataSource()->request().url().isValid()) )
    return true;

  return false;
}
}

void RunScriptsAtDocumentStart(blink::WebLocalFrame* frame) {
  if (!InjectAllowed(frame))
    return;

  blink::WebString &browser_js = OpScriptStore::GetSingleton()->browser_js();
  bool enabled = OpScriptStore::GetSingleton()->browserjs_enabled();

  if (browser_js.length() && enabled) {
    blink::WebScriptSource script(browser_js,
                                  GURL("file://localhost/browser.js"));

    frame->executeScript(script);
  }

  blink::WebString &user_js = OpScriptStore::GetSingleton()->user_js();

  if (user_js.length()) {
    blink::WebScriptSource script(user_js,
                                  GURL("file://localhost/user.js"));

    frame->executeScript(script);
  }
}

}  // namespace op_script_injector_utils
