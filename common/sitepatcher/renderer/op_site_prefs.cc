// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/renderer/op_site_prefs.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "common/message_generator/all_messages.h"
#include "content/public/common/user_agent.h"
#include "content/public/renderer/render_thread.h"

OpSitePrefs::OpSitePrefs()
    : prefs_(),
      prefs_lock_(),
      siteprefs_enabled_(true) {
  content::RenderThread::Get()->AddObserver(this);
}

OpSitePrefs::~OpSitePrefs() {
}

std::string OpSitePrefs::GetUserAgent(const GURL& url) {
  if (!siteprefs_enabled_)
    return "";

  base::AutoLock lock(prefs_lock_);
  std::string host = url.host();
  while (host.find(".") != std::string::npos) {
    std::map<std::string, SitePref>::const_iterator pref = prefs_.find(host);
    if (pref != prefs_.end())
      return pref->second.user_agent;
    host = host.substr(host.find(".") + 1);
  }
  return "";
}

void OpSitePrefs::ProcessPrefsDict(const base::Value* prefs_val) {
  const base::DictionaryValue* prefs = NULL;
  prefs_val->GetAsDictionary(&prefs);
  if (!prefs)
    return;

  const base::DictionaryValue *pref_info = NULL;
  const base::ListValue *host_list = NULL;
  const base::ListValue *ua_list = NULL;
  if (!prefs->GetDictionary("preferences", &pref_info) ||
      !pref_info->GetList("host", &host_list) ||
      !pref_info->GetList("config.useragent", &ua_list))
    return;

  prefs_.clear();
  for (const auto& host : *host_list)
    ProcessHostDict(host.get(), ua_list);
}

void OpSitePrefs::ProcessHostDict(
    const base::Value* host_val,
    const base::ListValue* ua_list) {
  const base::DictionaryValue *host_info = NULL;
  host_val->GetAsDictionary(&host_info);
  if (!host_info)
    return;

  std::string host_name;
  if (!host_info->GetString("@name", &host_name))
    return;

  SitePref pref;
  pref.user_agent = GetUserAgentForHostInfo(host_info, ua_list);
  prefs_.insert(std::make_pair(host_name, pref));
}

std::string OpSitePrefs::GetUserAgentForHostInfo(
    const base::DictionaryValue* host_info,
    const base::ListValue* ua_list) {
  std::string spoof_ua("");
  const base::ListValue *host_section_list = NULL;
  const base::DictionaryValue *host_section_info = NULL;
  if (host_info->GetDictionary("section", &host_section_info))
    spoof_ua = GetUserAgentForHostSection(host_section_info, ua_list);

  if (host_info->GetList("section", &host_section_list)) {
    for (base::ListValue::const_iterator j = host_section_list->begin();
        j != host_section_list->end(); ++j) {
      if (!(*j)->GetAsDictionary(&host_section_info))
        continue;

      spoof_ua = GetUserAgentForHostSection(host_section_info, ua_list);
      if (!spoof_ua.empty())
        break;
    }
  }
  return spoof_ua;
}

std::string OpSitePrefs::GetUserAgentForHostSection(
    const base::DictionaryValue* host_section_info,
    const base::ListValue* ua_list) const {
  std::string host_section_name;
  const base::DictionaryValue *host_section_pref_info;
  std::string pref_name;
  std::string spoof_id;
  if (!host_section_info->GetString("@name", &host_section_name) ||
      host_section_name.compare("User Agent") != 0 ||
      !host_section_info->GetDictionary("pref", &host_section_pref_info) ||
      !host_section_pref_info->GetString("@name", &pref_name) ||
      pref_name.compare("Spoof UserAgent ID") != 0 ||
      !host_section_pref_info->GetString("@value", &spoof_id))
    return "";

  for (base::ListValue::const_iterator i = ua_list->begin();
      i != ua_list->end(); ++i) {
    std::string spoof_ua;
    const base::DictionaryValue* ua_info;
    std::string ua_id;
    if (!(*i)->GetAsDictionary(&ua_info) ||
        !ua_info->GetString("@id", &ua_id) ||
        ua_id.compare(spoof_id) != 0 ||
        !ua_info->GetString("@value", &spoof_ua))
      continue;

    ReplacePlaceholder(spoof_ua, "$PLATFORM", content::BuildOSCpuInfo());
    ReplacePlaceholder(spoof_ua, "$OPR", "OPR/" OPR_VERSION);

    return spoof_ua;
  }

  return "";
}

void OpSitePrefs::ReplacePlaceholder(
    std::string& str,
    const std::string placeholder,
    const std::string value) const {
  size_t placeholder_idx = str.find(placeholder, 0);
  if (placeholder_idx != std::string::npos)
      str.replace(placeholder_idx, placeholder.length(), value);
}

bool OpSitePrefs::OnControlMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(OpSitePrefs, message)
    IPC_MESSAGE_HANDLER(OpMsg_SitePrefsUpdated, OnSitePrefsUpdated)
    IPC_MESSAGE_HANDLER(OpMsg_SitePrefsEnable, OnSitePrefsEnabled)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void OpSitePrefs::OnSitePrefsUpdated(base::SharedMemoryHandle handle,
                                     size_t size) {
  base::AutoLock lock(prefs_lock_);
  DVLOG(1) << "Received SitePrefs update notification.";

  if (!base::SharedMemory::IsHandleValid(handle))
    return;

  base::SharedMemory shared_memory(handle, true);

  if (!shared_memory.Map(size))
    return;

  std::string prefs_string;

  prefs_string.assign(
    static_cast<const std::string::value_type*>(shared_memory.memory()),
    size / sizeof(std::string::value_type));

  std::unique_ptr<base::Value> prefs_val(base::JSONReader::Read(prefs_string));
  if (prefs_val && prefs_val->IsType(base::Value::TYPE_DICTIONARY))
    ProcessPrefsDict(prefs_val.get());
}

void OpSitePrefs::OnSitePrefsEnabled(bool enabled) {
  siteprefs_enabled_ = enabled;
}
