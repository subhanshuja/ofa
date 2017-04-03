// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHROME_BROWSER_NET_TURBO_SETTINGS_H_
#define CHROME_BROWSER_NET_TURBO_SETTINGS_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "components/keyed_service/core/refcounted_keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

class TurboSettings : public RefcountedKeyedService {
 public:
  enum class TurboModeType { TURBO1, TURBO2 };

  enum class WebContentsTurboSettingType {
    USE_GLOBAL_SETTING,
    ENABLE,
    DISABLE
  };

  explicit TurboSettings(PrefService* prefs);

  void SetTurboMode(TurboModeType mode);
  void SetTurbo2VideoCompression(bool enabled);
  void SetTurbo2AdblockBlank(bool enabled);
  void SetForWebContents(content::WebContents* web_contents,
                         WebContentsTurboSettingType setting);
  bool IsTurboEnabled(content::WebContents* web_contents);

  bool IsFrameHostUsingTurbo(int render_process_id, int render_frame_id);

  void ShutdownOnUIThread() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

 private:
  typedef std::pair<int, int> RenderFrameHostID;
  typedef std::vector<RenderFrameHostID> RenderFrameHostIDs;

  // Stores turbo state which should be used for specific WebContents
  // if the setting was overriden for it.
  // Helper to keep track of RenderFrameHosts associated with the WebContents.
  // This info is used by TurboSettings on IO thread to properly identify all
  // requests originating for this WebContent's RenderFrameHosts.
  class WebContentsTurboSetting;

  ~TurboSettings() override;
  void TurboPrefChanged();
  void DetermineTurboUsage();
  void ApplySettings();

  PrefChangeRegistrar pref_change_registrar_;

  // Global settings of turbo.
  TurboModeType turbo_mode_;
  bool use_turbo2_video_compression_;
  bool use_turbo2_adblock_blank_;

  // Whether turbo is actually in use. This value is combination of global
  // setting and per WebContents settings. Set to true when turbo is turned on
  // globally or is turned on for any WebContents.
  bool turbo_in_use_;

  std::vector<std::unique_ptr<WebContentsTurboSetting>>
      web_contents_turbo_settings_;

  // Used around accesses to the turbo settings read on IO thread to guarantee
  // thread safety.
  mutable base::Lock lock_;
  bool turbo_enabled_;
  // Currently these are simple vectors of int pairs for both states.
  RenderFrameHostIDs turbo_enabled_host_ids_;
  RenderFrameHostIDs turbo_disabled_host_ids_;

  DISALLOW_COPY_AND_ASSIGN(TurboSettings);
};

#endif  // CHROME_BROWSER_NET_TURBO_SETTINGS_H_
