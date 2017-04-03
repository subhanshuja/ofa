// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_RENDERER_OP_SITE_PREFS_H_
#define COMMON_SITEPATCHER_RENDERER_OP_SITE_PREFS_H_

#include <string>
#include <map>
#include <memory>

#include "base/memory/shared_memory.h"
#include "base/synchronization/lock.h"
#include "content/public/renderer/render_thread_observer.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}

struct SitePref {
  std::string user_agent;
};

class OpSitePrefs : public content::RenderThreadObserver {
 public:
  OpSitePrefs();
  ~OpSitePrefs() override;

  std::string GetUserAgent(const GURL& url);

  // Inherited from RenderProcessObserver
  bool OnControlMessageReceived(const IPC::Message& message) override;

 private:
  void ProcessPrefsDict(const base::Value* prefs_val);
  void ProcessHostDict(const base::Value* host_val,
                       const base::ListValue* ua_list);

  std::string GetUserAgentForHostInfo(const base::DictionaryValue* host_info,
                                      const base::ListValue* ua_list);
  std::string GetUserAgentForHostSection(
      const base::DictionaryValue* host_section_info,
      const base::ListValue* ua_list) const;

  // replaces first instance (if any) of |placeholder| in |str| with |value|
  void ReplacePlaceholder(std::string& str,
                          const std::string placeholder,
                          const std::string value) const;

  void OnSitePrefsUpdated(base::SharedMemoryHandle handle, size_t size);
  void OnSitePrefsEnabled(bool enabled);

  std::map<std::string, SitePref> prefs_;
  base::Lock prefs_lock_;
  bool siteprefs_enabled_;

  DISALLOW_COPY_AND_ASSIGN(OpSitePrefs);
};

#endif  // COMMON_SITEPATCHER_RENDERER_OP_SITE_PREFS_H_
