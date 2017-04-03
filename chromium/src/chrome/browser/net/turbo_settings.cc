// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chrome/browser/net/turbo_settings.h"

#include <algorithm>

#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/http/http_stream_factory.h"
#include "net/turbo/turbo_session_pool.h"

namespace {

const char kTurboSwitch[] = "turbo";
const char kUseTurbo2Switch[] = "use-turbo2";
const char kUseTurbo2VideoCompressionSwitch[] = "use-turbo2-video-compression";
const char kUseTurbo2CompressSSLSwitch[] = "use-turbo2-compress-ssl";
const char kUseTurbo2AdblockBlank[] = "use-turbo2-adblock-blank";

}  // namespace

// Stores turbo state which should be used for specific WebContents
// if the setting was overriden for it.
// Helps to keep track of RenderFrameHosts associated with the WebContents.
// This info is used by TurboSettings on IO thread to properly identify all
// requests originating for this WebContent's RenderFrameHosts.
class TurboSettings::WebContentsTurboSetting
    : public content::WebContentsObserver {
 public:
  WebContentsTurboSetting(TurboSettings* turbo_settings,
                          content::WebContents* web_contents,
                          bool enabled);
  ~WebContentsTurboSetting() override;

  void set_enabled(bool enabled);
  bool enabled() const;

 private:
  RenderFrameHostIDs* GetHostIDContainerForStatus(bool enabled) const;
  static RenderFrameHostID IDFromHost(content::RenderFrameHost* host);

  void RenderFrameEnumerated(RenderFrameHostIDs* add_to,
                             RenderFrameHostIDs* remove_from,
                             content::RenderFrameHost* render_frame_host);

  // content::WebContentsObserver overrides.
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void WebContentsDestroyed() override;

  TurboSettings* turbo_settings_;
  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsTurboSetting);
};

TurboSettings::WebContentsTurboSetting::WebContentsTurboSetting(
    TurboSettings* turbo_settings,
    content::WebContents* web_contents,
    bool enabled)
    : content::WebContentsObserver(web_contents),
      turbo_settings_(turbo_settings),
      enabled_(enabled) {
  base::AutoLock auto_lock(turbo_settings_->lock_);
  web_contents->ForEachFrame(base::Bind(
      &TurboSettings::WebContentsTurboSetting::RenderFrameEnumerated,
      base::Unretained(this), GetHostIDContainerForStatus(enabled), nullptr));
}

TurboSettings::WebContentsTurboSetting::~WebContentsTurboSetting() {
  base::AutoLock auto_lock(turbo_settings_->lock_);
  web_contents()->ForEachFrame(base::Bind(
      &TurboSettings::WebContentsTurboSetting::RenderFrameEnumerated,
      base::Unretained(this), nullptr, GetHostIDContainerForStatus(enabled())));
}

void TurboSettings::WebContentsTurboSetting::set_enabled(bool enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (enabled_ == enabled)
    return;
  enabled_ = enabled;
  base::AutoLock auto_lock(turbo_settings_->lock_);
  web_contents()->ForEachFrame(
      base::Bind(&TurboSettings::WebContentsTurboSetting::RenderFrameEnumerated,
                 base::Unretained(this), GetHostIDContainerForStatus(enabled),
                 GetHostIDContainerForStatus(!enabled)));
}

bool TurboSettings::WebContentsTurboSetting::enabled() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return enabled_;
}

TurboSettings::RenderFrameHostIDs*
TurboSettings::WebContentsTurboSetting::GetHostIDContainerForStatus(
    bool enabled) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return enabled ? &turbo_settings_->turbo_enabled_host_ids_
                 : &turbo_settings_->turbo_disabled_host_ids_;
}

TurboSettings::RenderFrameHostID
TurboSettings::WebContentsTurboSetting::IDFromHost(
    content::RenderFrameHost* host) {
  return std::make_pair(host->GetProcess()->GetID(), host->GetRoutingID());
}

void TurboSettings::WebContentsTurboSetting::RenderFrameEnumerated(
    RenderFrameHostIDs* add_to,
    RenderFrameHostIDs* remove_from,
    content::RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // This function is used to do bulk updates, lock is hold by the caller.
  RenderFrameHostID frame_id = IDFromHost(render_frame_host);
  if (remove_from) {
    remove_from->erase(
        std::remove(remove_from->begin(), remove_from->end(), frame_id),
        remove_from->end());
  }
  if (add_to) {
    add_to->push_back(frame_id);
  }
}

void TurboSettings::WebContentsTurboSetting::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  RenderFrameHostIDs* host_ids = GetHostIDContainerForStatus(enabled());
  RenderFrameHostID host_id = IDFromHost(render_frame_host);
  // We only do updates of host_ids on UI thread, since find below is only
  // reading the data race condition is not possible so there is no need to
  // hold lock here yet, we will hold it when updating only.
  auto iter = std::find(host_ids->begin(), host_ids->end(), host_id);
  if (iter == host_ids->end()) {
    base::AutoLock auto_lock(turbo_settings_->lock_);
    host_ids->emplace_back(host_id);
  }
}

void TurboSettings::WebContentsTurboSetting::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  RenderFrameHostIDs* host_ids = GetHostIDContainerForStatus(enabled());
  // We only do updates of host_ids on UI thread, since find below is only
  // reading the data race condition is not possible so there is no need to
  // hold lock here yet, we will hold it when updating only.
  auto iter = std::find(host_ids->begin(), host_ids->end(),
                        IDFromHost(render_frame_host));
  if (iter != host_ids->end()) {
    base::AutoLock auto_lock(turbo_settings_->lock_);
    host_ids->erase(iter);
  }
}

void TurboSettings::WebContentsTurboSetting::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  RenderFrameHostIDs* host_ids = GetHostIDContainerForStatus(enabled());
  if (old_host) {
    // We only do updates of host_ids on UI thread, since find below is only
    // reading the data race condition is not possible so there is no need to
    // hold lock here yet, we will hold it when updating only.
    auto old_iter =
        std::find(host_ids->begin(), host_ids->end(), IDFromHost(old_host));
    DCHECK(old_iter != host_ids->end());
    base::AutoLock auto_lock(turbo_settings_->lock_);
    host_ids->erase(old_iter);
  }
  RenderFrameHostID new_host_id = IDFromHost(new_host);
  // We only do updates of host_ids on UI thread, since find below is only
  // reading the data race condition is not possible so there is no need to
  // hold lock here yet, we will hold it when updating only.
  auto new_iter = std::find(host_ids->begin(), host_ids->end(), new_host_id);
  if (new_iter == host_ids->end()) {
    base::AutoLock auto_lock(turbo_settings_->lock_);
    host_ids->emplace_back(new_host_id);
  }
}

void TurboSettings::WebContentsTurboSetting::WebContentsDestroyed() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  turbo_settings_->SetForWebContents(
      web_contents(), WebContentsTurboSettingType::USE_GLOBAL_SETTING);
}


TurboSettings::TurboSettings(PrefService* prefs) : turbo_in_use_(false) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kTurboSwitch))
    prefs->ClearPref(prefs::kTurboEnabled);

  turbo_enabled_ = prefs->GetBoolean(prefs::kTurboEnabled);

  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      prefs::kTurboEnabled,
      base::Bind(&TurboSettings::TurboPrefChanged, base::Unretained(this)));

  turbo_mode_ =
      base::CommandLine::ForCurrentProcess()->HasSwitch(kUseTurbo2Switch)
          ? TurboModeType::TURBO2
          : TurboModeType::TURBO1;
  use_turbo2_video_compression_ =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          kUseTurbo2VideoCompressionSwitch);

  use_turbo2_adblock_blank_ =
      base::CommandLine::ForCurrentProcess()->HasSwitch(kUseTurbo2AdblockBlank);

  bool use_turbo2_compress_ssl =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          kUseTurbo2CompressSSLSwitch);
  net::TurboSessionPool::SetCompresSsl(use_turbo2_compress_ssl);

  DetermineTurboUsage();
  ApplySettings();
}

TurboSettings::~TurboSettings() {}

void TurboSettings::ShutdownOnUIThread() {
  pref_change_registrar_.RemoveAll();
  DCHECK(web_contents_turbo_settings_.empty());
  DCHECK(turbo_enabled_host_ids_.empty());
  DCHECK(turbo_disabled_host_ids_.empty());
}

// static
void TurboSettings::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kTurboEnabled,
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kTurboSwitch) == "on");
}

void TurboSettings::TurboPrefChanged() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  {
    base::AutoLock auto_lock(lock_);
    turbo_enabled_ =
        pref_change_registrar_.prefs()->GetBoolean(prefs::kTurboEnabled);
  }
  DetermineTurboUsage();
}

void TurboSettings::DetermineTurboUsage() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bool was_turbo_in_use = turbo_in_use_;
  turbo_in_use_ =
      turbo_enabled_ ||
      std::find_if(web_contents_turbo_settings_.begin(),
                   web_contents_turbo_settings_.end(),
                   [](const std::unique_ptr<WebContentsTurboSetting>& setting){
                     return setting->enabled();
                   }) != web_contents_turbo_settings_.end();
  if (was_turbo_in_use != turbo_in_use_)
    ApplySettings();
}

void TurboSettings::ApplySettings() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bool use_turbo2 = turbo_mode_ == TurboModeType::TURBO2;
  net::HttpStreamFactory::set_turbo1_enabled(turbo_in_use_ && !use_turbo2);
  net::HttpStreamFactory::set_turbo2_enabled(turbo_in_use_ && use_turbo2);
  net::HttpStreamFactory::set_turbo2_over_http2_enabled(
      turbo_in_use_ && use_turbo2 && use_turbo2_over_http2_);
  net::TurboSessionPool::SetVideoCompressionEnabled(
      turbo_in_use_ && use_turbo2 && use_turbo2_video_compression_);
  net::TurboSessionPool::SetAdblockBlankEnabled(
      turbo_in_use_ && use_turbo2 && use_turbo2_adblock_blank_);
}

void TurboSettings::SetTurboMode(TurboModeType mode) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (turbo_mode_ == mode)
    return;
  turbo_mode_ = mode;
  ApplySettings();
}

void TurboSettings::SetTurbo2VideoCompression(bool enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  use_turbo2_video_compression_ = enabled;
  ApplySettings();
}

void TurboSettings::SetTurbo2AdblockBlank(bool enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  use_turbo2_adblock_blank_ = enabled;
  ApplySettings();
}

void TurboSettings::SetForWebContents(content::WebContents* web_contents,
                                      WebContentsTurboSettingType setting) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto iter = std::find_if(
      web_contents_turbo_settings_.begin(), web_contents_turbo_settings_.end(),
      [web_contents](const std::unique_ptr<WebContentsTurboSetting>& wc_setting) {
        return wc_setting->web_contents() == web_contents;
      });
  if (iter != web_contents_turbo_settings_.end()) {
    if (setting == WebContentsTurboSettingType::USE_GLOBAL_SETTING)
      web_contents_turbo_settings_.erase(iter);
    else
      (*iter)->set_enabled(setting == WebContentsTurboSettingType::ENABLE);
  } else {
    if (setting == WebContentsTurboSettingType::USE_GLOBAL_SETTING)
      return;
    web_contents_turbo_settings_.emplace_back(
        base::WrapUnique(new WebContentsTurboSetting(
            this, web_contents,
            setting == WebContentsTurboSettingType::ENABLE)));
  }
  DetermineTurboUsage();
}

bool TurboSettings::IsTurboEnabled(content::WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto iter = std::find_if(
      web_contents_turbo_settings_.begin(), web_contents_turbo_settings_.end(),
      [web_contents](const std::unique_ptr<WebContentsTurboSetting>& wc_setting) {
        return wc_setting->web_contents() == web_contents;
      });
  if (iter != web_contents_turbo_settings_.end())
    return (*iter)->enabled();
  return turbo_enabled_;
}

bool TurboSettings::IsFrameHostUsingTurbo(int render_process_id,
                                          int render_frame_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  base::AutoLock auto_lock(lock_);
  if (turbo_enabled_)
    return std::find(turbo_disabled_host_ids_.begin(),
                     turbo_disabled_host_ids_.end(),
                     std::make_pair(render_process_id, render_frame_id)) ==
           turbo_disabled_host_ids_.end();
  return std::find(turbo_enabled_host_ids_.begin(),
                   turbo_enabled_host_ids_.end(),
                   std::make_pair(render_process_id, render_frame_id)) !=
         turbo_enabled_host_ids_.end();
}
