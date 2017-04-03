// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_RENDERER_OP_SCRIPT_INJECTOR_H_
#define COMMON_SITEPATCHER_RENDERER_OP_SCRIPT_INJECTOR_H_

namespace blink {
class WebLocalFrame;
}

namespace op_script_injector_utils {
void RunScriptsAtDocumentStart(blink::WebLocalFrame* frame);
};

#endif  // COMMON_SITEPATCHER_RENDERER_OP_SCRIPT_INJECTOR_H_
