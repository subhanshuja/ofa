// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%include "stdint.i"

typedef int64_t int64;

#define DECLARE_CLASS(C) \
  class C { \
  private: \
    C(); \
    ~C(); \
  };

namespace content {
DECLARE_CLASS(BrowserContext)
DECLARE_CLASS(SiteInstance)
}  // namespace content

namespace gfx {
DECLARE_CLASS(Rect)
}  // namespace gfx

namespace net {
DECLARE_CLASS(AuthChallengeInfo)
DECLARE_CLASS(URLRequest)
}

namespace blink {
DECLARE_CLASS(WebLayer)
}
