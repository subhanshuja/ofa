// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_render_process_observer.cc
// @final-synchronized

#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_thread.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"

#include "chill/renderer/opera_render_process_observer.h"

using blink::WebSecurityPolicy;
using blink::WebString;

namespace opera {

const char kOperaUIPageScheme[] = "operaui";

OperaRenderProcessObserver::OperaRenderProcessObserver() {
  content::RenderThread::Get()->AddObserver(this);

  // operaui:// pages are handled by the UI, we just need to make sure an empty
  // page is loaded whenever a page is requsted.
  WebSecurityPolicy::registerURLSchemeAsEmptyDocument(
      WebString(base::ASCIIToUTF16(kOperaUIPageScheme)));
}

OperaRenderProcessObserver::~OperaRenderProcessObserver() {
}

}  // namespace opera
