// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_RENDERER_OP_SCRIPT_STORE_H_
#define COMMON_SITEPATCHER_RENDERER_OP_SCRIPT_STORE_H_

#include "base/compiler_specific.h"
#include "base/memory/shared_memory.h"
#include "content/public/renderer/render_thread_observer.h"
#include "third_party/WebKit/public/platform/WebString.h"

class OpScriptStore : public content::RenderThreadObserver {
 public:
  OpScriptStore();

  // There can be only one per process.
  static OpScriptStore* GetSingleton();

  blink::WebString& browser_js() { return browser_js_; }
  blink::WebString& user_js() { return user_js_; }
  bool browserjs_enabled() { return browserjs_enabled_; }

  // Inherited from RenderProcessObserver
  bool OnControlMessageReceived(const IPC::Message& message) override;
  void OnRenderProcessShutdown() override;

 private:
  void OnBrowserJsUpdated(base::SharedMemoryHandle handle, size_t size);
  void OnSetUserJs(base::SharedMemoryHandle handle, size_t size);
  void OnBrowserJsEnabled(bool enabled);

  static OpScriptStore* s_singleton;

  blink::WebString browser_js_;
  blink::WebString user_js_;
  
  bool browserjs_enabled_;

  DISALLOW_COPY_AND_ASSIGN(OpScriptStore);
};

#endif  // COMMON_SITEPATCHER_RENDERER_OP_SCRIPT_STORE_H_
