// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/renderer/op_script_store.h"

#include "common/message_generator/all_messages.h"
#include "content/public/renderer/render_thread.h"

OpScriptStore* OpScriptStore::s_singleton = NULL;

OpScriptStore::OpScriptStore() :
    browserjs_enabled_(true) {
  DCHECK(!s_singleton);
  s_singleton = this;

  content::RenderThread::Get()->AddObserver(this);
}

OpScriptStore* OpScriptStore::GetSingleton() {
  return s_singleton;
}

bool OpScriptStore::OnControlMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(OpScriptStore, message)
    IPC_MESSAGE_HANDLER(OpMsg_BrowserJsUpdated, OnBrowserJsUpdated)
    IPC_MESSAGE_HANDLER(OpMsg_SetUserJs, OnSetUserJs)
    IPC_MESSAGE_HANDLER(OpMsg_BrowserJsEnable, OnBrowserJsEnabled)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void OpScriptStore::OnRenderProcessShutdown() {
  // RenderProcess is about to shut down blink, which in turn will destroy
  // PartitionAllocator that provides storage for WebStrings. browser_js_
  // must be reset before that happens, otherwise it may crash later.
  browser_js_.reset();
}

void OpScriptStore::OnBrowserJsUpdated(base::SharedMemoryHandle handle,
                                       size_t size) {
  DVLOG(3) << "Received Browser JS update notification.";

  if (!base::SharedMemory::IsHandleValid(handle))
    return;

  base::SharedMemory shared_memory(handle, true);

  if (!shared_memory.Map(size))
    return;

  browser_js_.assign(
      static_cast<const blink::WebUChar*>(shared_memory.memory()),
      size / sizeof(base::char16));
}

void OpScriptStore::OnSetUserJs(base::SharedMemoryHandle handle, size_t size) {
  DVLOG(3) << "Received User Js.";

  if (!base::SharedMemory::IsHandleValid(handle))
    return;

  base::SharedMemory shared_memory(handle, true);

  if (!shared_memory.Map(size))
    return;

  user_js_.assign(
      static_cast<const blink::WebUChar*>(shared_memory.memory()),
      size / sizeof(base::char16));
}

void OpScriptStore::OnBrowserJsEnabled(bool enabled) {
  browserjs_enabled_ = enabled;
}
