// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/about_flags.cc
// @last-synchronized ba84619e63fb09545be28988bad3c8ddb5fb6adc

#include "chill/browser/about_flags.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <utility>

#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "cc/base/switches.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/flags_ui/flags_ui_switches.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/common/content_switches.h"
#include "grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_switches.h"
#include "ui/events/event_switches.h"
#include "ui/gfx/switches.h"
#include "ui/gl/gl_switches.h"
#include "ui/keyboard/keyboard_switches.h"

#include "opera_common/grit/product_free_common_strings.h"
#include "opera_common/grit/product_related_common_strings.h"

#if defined(OS_CHROMEOS)
#include "chromeos/chromeos_switches.h"
#include "third_party/cros_system_api/switches/chrome_switches.h"
#endif

#include "chill/browser/flags_json_storage.h"
#include "chill/common/opera_switches.h"


using base::UserMetricsAction;
using opera::FlagsJsonStorage;

namespace about_flags {

// Macros to simplify specifying the type.
#define SINGLE_VALUE_TYPE_AND_VALUE(command_line_switch, switch_value) \
    Experiment::SINGLE_VALUE, \
    command_line_switch, switch_value, NULL, NULL, NULL, 0
#define SINGLE_VALUE_TYPE(command_line_switch) \
    SINGLE_VALUE_TYPE_AND_VALUE(command_line_switch, "")
#define SINGLE_DISABLE_VALUE_TYPE_AND_VALUE(command_line_switch, switch_value) \
    Experiment::SINGLE_DISABLE_VALUE, \
      command_line_switch, switch_value, NULL, NULL, NULL, 0
#define SINGLE_DISABLE_VALUE_TYPE(command_line_switch) \
    SINGLE_DISABLE_VALUE_TYPE_AND_VALUE(command_line_switch, "")
#define ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(enable_switch, enable_value, \
                                            disable_switch, disable_value) \
    Experiment::ENABLE_DISABLE_VALUE, enable_switch, enable_value, \
    disable_switch, disable_value, NULL, 3
#define ENABLE_DISABLE_VALUE_TYPE(enable_switch, disable_switch) \
    ENABLE_DISABLE_VALUE_TYPE_AND_VALUE(enable_switch, "", disable_switch, "")
#define MULTI_VALUE_TYPE(choices) \
    Experiment::MULTI_VALUE, NULL, NULL, NULL, NULL, choices, arraysize(choices)

namespace {

const unsigned kOsAll = kOsMac | kOsWin | kOsLinux | kOsCrOS | kOsAndroid;
const unsigned kOsDesktop = kOsMac | kOsWin | kOsLinux | kOsCrOS;

// Adds a |StringValue| to |list| for each platform where |bitmask| indicates
// whether the experiment is available on that platform.
void AddOsStrings(unsigned bitmask, base::ListValue* list) {
  struct {
    unsigned bit;
    const char* const name;
  } kBitsToOs[] = {
    {kOsMac, "Mac"},
    {kOsWin, "Windows"},
    {kOsLinux, "Linux"},
    {kOsCrOS, "Chrome OS"},
    {kOsAndroid, "Android"},
    {kOsCrOSOwnerOnly, "Chrome OS (owner only)"},
  };
  for (size_t i = 0; i < arraysize(kBitsToOs); ++i)
    if (bitmask & kBitsToOs[i].bit)
      list->Append(new base::StringValue(kBitsToOs[i].name));
}

// Convert switch constants to proper base::CommandLine::StringType strings.
base::CommandLine::StringType GetSwitchString(const std::string& flag) {
  base::CommandLine cmd_line(base::CommandLine::NO_PROGRAM);
  cmd_line.AppendSwitch(flag);
  DCHECK_EQ(cmd_line.argv().size(), 2U);
  return cmd_line.argv()[1];
}

// Scoops flags from a command line.
std::set<base::CommandLine::StringType> ExtractFlagsFromCommandLine(
    const base::CommandLine& cmdline) {
  std::set<base::CommandLine::StringType> flags;
  // First do the ones between --flag-switches-begin and --flag-switches-end.
  base::CommandLine::StringVector::const_iterator first =
      std::find(cmdline.argv().begin(), cmdline.argv().end(),
                GetSwitchString(switches::kFlagSwitchesBegin));
  base::CommandLine::StringVector::const_iterator last =
      std::find(cmdline.argv().begin(), cmdline.argv().end(),
                GetSwitchString(switches::kFlagSwitchesEnd));
  if (first != cmdline.argv().end() && last != cmdline.argv().end())
    flags.insert(first + 1, last);
#if defined(OS_CHROMEOS)
  // Then add those between --policy-switches-begin and --policy-switches-end.
  first = std::find(cmdline.argv().begin(), cmdline.argv().end(),
                    GetSwitchString(chromeos::switches::kPolicySwitchesBegin));
  last = std::find(cmdline.argv().begin(), cmdline.argv().end(),
                   GetSwitchString(chromeos::switches::kPolicySwitchesEnd));
  if (first != cmdline.argv().end() && last != cmdline.argv().end())
    flags.insert(first + 1, last);
#endif
  return flags;
}

const Experiment::Choice kTouchEventsChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_AUTOMATIC, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    switches::kTouchEvents,
    switches::kTouchEventsEnabled },
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kTouchEvents,
    switches::kTouchEventsDisabled }
};

const Experiment::Choice kDefaultTileWidthChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_DEFAULT_TILE_WIDTH_SHORT,
    switches::kDefaultTileWidth, "128"},
  { IDS_FLAGS_DEFAULT_TILE_WIDTH_TALL,
    switches::kDefaultTileWidth, "256"},
  { IDS_FLAGS_DEFAULT_TILE_WIDTH_GRANDE,
    switches::kDefaultTileWidth, "512"},
  { IDS_FLAGS_DEFAULT_TILE_WIDTH_VENTI,
    switches::kDefaultTileWidth, "1024"}
};

const Experiment::Choice kDefaultTileHeightChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_FLAGS_DEFAULT_TILE_HEIGHT_SHORT,
    switches::kDefaultTileHeight, "128"},
  { IDS_FLAGS_DEFAULT_TILE_HEIGHT_TALL,
    switches::kDefaultTileHeight, "256"},
  { IDS_FLAGS_DEFAULT_TILE_HEIGHT_GRANDE,
    switches::kDefaultTileHeight, "512"},
  { IDS_FLAGS_DEFAULT_TILE_HEIGHT_VENTI,
    switches::kDefaultTileHeight, "1024"}
};

const Experiment::Choice kTileCompressionChoices[] = {
  { IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT, "", "" },
  { IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
    cc::switches::kEnableTileCompression, ""},
  { IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    switches::kDisableTileCompression, ""}
};

const Experiment::Choice kTileCompressionHeuristicChoices[] = {
  { IDS_FLAGS_TILE_COMPRESSION_HEURISTIC_DEFAULT, "", "" },
  { IDS_FLAGS_TILE_COMPRESSION_HEURISTIC_NONE,
    cc::switches::kTileCompressionHeuristic, "0" },
  { IDS_FLAGS_TILE_COMPRESSION_HEURISTIC_GRADIENT,
    cc::switches::kTileCompressionHeuristic, "1" },
  { IDS_FLAGS_TILE_COMPRESSION_HEURISTIC_IMAGE,
    cc::switches::kTileCompressionHeuristic, "2" },
  { IDS_FLAGS_TILE_COMPRESSION_HEURISTIC_GRADIENT_AND_IMAGE,
    cc::switches::kTileCompressionHeuristic, "3" },
};

// RECORDING USER METRICS FOR FLAGS:
// -----------------------------------------------------------------------------
// The first line of the experiment is the internal name. If you'd like to
// gather statistics about the usage of your flag, you should append a marker
// comment to the end of the feature name, like so:
//   "my-special-feature",  // FLAGS:RECORD_UMA
//
// After doing that, run //chrome/tools/extract_actions.py (see instructions at
// the top of that file for details) to update the chromeactions.txt file, which
// will enable UMA to record your feature flag.
//
// After your feature has shipped under a flag, you can locate the metrics under
// the action name AboutFlags_internal-action-name. Actions are recorded once
// per startup, so you should divide this number by AboutFlags_StartupTick to
// get a sense of usage. Note that this will not be the same as number of users
// with a given feature enabled because users can quit and relaunch the
// application multiple times over a given time interval. The dashboard also
// shows you how many (metrics reporting) users have enabled the flag over the
// last seven days. However, note that this is not the same as the number of
// users who have the flag enabled, since enabling the flag happens once,
// whereas running with the flag enabled happens until the user flips the flag
// again.

// To add a new experiment add to the end of kExperiments. There are two
// distinct types of experiments:
// . SINGLE_VALUE: experiment is either on or off. Use the SINGLE_VALUE_TYPE
//   macro for this type supplying the command line to the macro.
// . MULTI_VALUE: a list of choices, the first of which should correspond to a
//   deactivated state for this lab (i.e. no command line option).  To specify
//   this type of experiment use the macro MULTI_VALUE_TYPE supplying it the
//   array of choices.
// See the documentation of Experiment for details on the fields.
//
// When adding a new choice, add it to the end of the list.
const Experiment kExperiments[] = {
  {
    "ignore-gpu-blacklist",
    IDS_FLAGS_IGNORE_GPU_BLACKLIST_NAME,
    IDS_FLAGS_IGNORE_GPU_BLACKLIST_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kIgnoreGpuBlacklist)
  },
  {
    "enable-experimental-canvas-features",
    IDS_FLAGS_EXPERIMENTAL_CANVAS_FEATURES_NAME,
    IDS_FLAGS_EXPERIMENTAL_CANVAS_FEATURES_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableExperimentalCanvasFeatures)
  },
  {
    "disable-accelerated-2d-canvas",
    IDS_FLAGS_ACCELERATED_2D_CANVAS_NAME,
    IDS_FLAGS_ACCELERATED_2D_CANVAS_DESCRIPTION,
    kOsAll,
    SINGLE_DISABLE_VALUE_TYPE(switches::kDisableAccelerated2dCanvas)
  },
  {
    "composited-layer-borders",
    IDS_FLAGS_COMPOSITED_LAYER_BORDERS,
    IDS_FLAGS_COMPOSITED_LAYER_BORDERS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(cc::switches::kShowCompositedLayerBorders)
  },
  {
    "show-autofill-type-predictions",
    IDS_FLAGS_SHOW_AUTOFILL_TYPE_PREDICTIONS_NAME,
    IDS_FLAGS_SHOW_AUTOFILL_TYPE_PREDICTIONS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(autofill::switches::kShowAutofillTypePredictions)
  },
  {
    "enable-javascript-harmony",
    IDS_FLAGS_JAVASCRIPT_HARMONY_NAME,
    IDS_FLAGS_JAVASCRIPT_HARMONY_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE_AND_VALUE(switches::kJavaScriptFlags, "--harmony")
  },
  {
    "enable-experimental-web-platform-features",
    IDS_FLAGS_EXPERIMENTAL_WEB_PLATFORM_FEATURES_NAME,
    IDS_FLAGS_EXPERIMENTAL_WEB_PLATFORM_FEATURES_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableExperimentalWebPlatformFeatures)
  },
  {
    "bypass-app-banner-engagement-checks",
    IDS_FLAGS_BYPASS_APP_BANNER_ENGAGEMENT_CHECKS_NAME,
    IDS_FLAGS_BYPASS_APP_BANNER_ENGAGEMENT_CHECKS_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kBypassAppBannerEngagementChecks)
  },
  {
    "default-tile-width",
    IDS_FLAGS_DEFAULT_TILE_WIDTH_NAME,
    IDS_FLAGS_DEFAULT_TILE_WIDTH_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kDefaultTileWidthChoices)
  },
  {
    "default-tile-height",
    IDS_FLAGS_DEFAULT_TILE_HEIGHT_NAME,
    IDS_FLAGS_DEFAULT_TILE_HEIGHT_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kDefaultTileHeightChoices)
  },
  {
    "tile-compression",
    IDS_FLAGS_ENABLE_TILE_COMPRESSION_NAME,
    IDS_FLAGS_ENABLE_TILE_COMPRESSION_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kTileCompressionChoices)
  },
  {
    "tile-compression-heuristic",
    IDS_FLAGS_TILE_COMPRESSION_HEURISTIC_NAME,
    IDS_FLAGS_TILE_COMPRESSION_HEURISTIC_DESCRIPTION,
    kOsAll,
    MULTI_VALUE_TYPE(kTileCompressionHeuristicChoices)
  },
#if defined(OS_ANDROID)
  {
    "disable-gesture-requirement-for-media-playback",
    IDS_FLAGS_GESTURE_REQUIREMENT_FOR_MEDIA_PLAYBACK_NAME,
    IDS_FLAGS_GESTURE_REQUIREMENT_FOR_MEDIA_PLAYBACK_DESCRIPTION,
    kOsAndroid,
    SINGLE_DISABLE_VALUE_TYPE(
        switches::kDisableGestureRequirementForMediaPlayback)
  },
#endif
  {
    "enable-tcp-fast-open",
    IDS_FLAGS_TCP_FAST_OPEN_NAME,
    IDS_FLAGS_TCP_FAST_OPEN_DESCRIPTION,
    kOsLinux | kOsCrOS | kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableTcpFastOpen)
  },
  {
    "enable-webgl-draft-extensions",
    IDS_FLAGS_WEBGL_DRAFT_EXTENSIONS_NAME,
    IDS_FLAGS_WEBGL_DRAFT_EXTENSIONS_DESCRIPTION,
    kOsAll,
    SINGLE_VALUE_TYPE(switches::kEnableWebGLDraftExtensions)
  },
  {
    "disable-pull-to-refresh-effect",
    IDS_FLAGS_PULL_TO_REFRESH_EFFECT_NAME,
    IDS_FLAGS_PULL_TO_REFRESH_EFFECT_DESCRIPTION,
    kOsAndroid,
    SINGLE_DISABLE_VALUE_TYPE(switches::kDisablePullToRefreshEffect)
  },
  {
    "enable-web-bluetooth",
    IDS_FLAGS_ENABLE_WEB_BLUETOOTH_NAME,
    IDS_FLAGS_ENABLE_WEB_BLUETOOTH_DESCRIPTION,
    kOsAndroid,
    SINGLE_VALUE_TYPE(switches::kEnableWebBluetooth)
  },
};

const Experiment* experiments = kExperiments;
size_t num_experiments = arraysize(kExperiments);

// Stores and encapsulates the little state that about:flags has.
class FlagsState {
 public:
  FlagsState() : needs_restart_(false) {}
  void ConvertFlagsToSwitches(
      const scoped_refptr<FlagsJsonStorage>& flags_storage,
      base::CommandLine* command_line);
  bool IsRestartNeededToCommitChanges();
  void SetFeatureEntryEnabled(
      const scoped_refptr<FlagsJsonStorage>& flags_storage,
      const std::string& internal_name,
      bool enable);
  void RemoveFlagsSwitches(
      std::map<std::string, base::CommandLine::StringType>* switch_list);
  void ResetAllFlags(const scoped_refptr<FlagsJsonStorage>& flags_storage);
  void reset();

  // Returns the singleton instance of this class
  static FlagsState* GetInstance() {
    return base::Singleton<FlagsState>::get();
  }

 private:
  bool needs_restart_;
  std::map<std::string, std::string> flags_switches_;

  DISALLOW_COPY_AND_ASSIGN(FlagsState);
};

// Adds the internal names for the specified experiment to |names|.
void AddInternalName(const Experiment& e, std::set<std::string>* names) {
  if (e.type == Experiment::SINGLE_VALUE ||
      e.type == Experiment::SINGLE_DISABLE_VALUE) {
    names->insert(e.internal_name);
  } else {
    DCHECK(e.type == Experiment::MULTI_VALUE ||
           e.type == Experiment::ENABLE_DISABLE_VALUE);
    for (int i = 0; i < e.num_choices; ++i)
      names->insert(e.NameForChoice(i));
  }
}

// Confirms that an experiment is valid, used in a DCHECK in
// SanitizeList below.
bool ValidateExperiment(const Experiment& e) {
  switch (e.type) {
    case Experiment::SINGLE_VALUE:
    case Experiment::SINGLE_DISABLE_VALUE:
      DCHECK_EQ(0, e.num_choices);
      DCHECK(!e.choices);
      break;
    case Experiment::MULTI_VALUE:
      DCHECK_GT(e.num_choices, 0);
      DCHECK(e.choices);
      DCHECK(e.choices[0].command_line_switch);
      DCHECK_EQ('\0', e.choices[0].command_line_switch[0]);
      break;
    case Experiment::ENABLE_DISABLE_VALUE:
      DCHECK_EQ(3, e.num_choices);
      DCHECK(!e.choices);
      DCHECK(e.command_line_switch);
      DCHECK(e.command_line_value);
      DCHECK(e.disable_command_line_switch);
      DCHECK(e.disable_command_line_value);
      break;
    default:
      NOTREACHED();
  }
  return true;
}

// Removes all experiments from the flag storage that are unknown, to prevent
// this list to become very long as experiments are added and removed.
void SanitizeList(const scoped_refptr<FlagsJsonStorage>& flags_storage) {
  std::set<std::string> known_experiments;
  for (size_t i = 0; i < num_experiments; ++i) {
    DCHECK(ValidateExperiment(experiments[i]));
    AddInternalName(experiments[i], &known_experiments);
  }

  std::set<std::string> enabled_experiments = flags_storage->GetFlags();

  std::set<std::string> new_enabled_experiments;
  std::set_intersection(
      known_experiments.begin(), known_experiments.end(),
      enabled_experiments.begin(), enabled_experiments.end(),
      std::inserter(new_enabled_experiments, new_enabled_experiments.begin()));

  if (new_enabled_experiments != enabled_experiments)
    flags_storage->SetFlags(new_enabled_experiments);
}

void GetSanitizedEnabledFlags(
    const scoped_refptr<FlagsJsonStorage>& flags_storage,
    std::set<std::string>* result) {
  SanitizeList(flags_storage);
  *result = flags_storage->GetFlags();
}

// Variant of GetSanitizedEnabledFlags that also removes any flags that aren't
// enabled on the current platform.
void GetSanitizedEnabledFlagsForCurrentPlatform(
    const scoped_refptr<FlagsJsonStorage>& flags_storage,
    std::set<std::string>* result) {
  GetSanitizedEnabledFlags(flags_storage, result);

  // Filter out any experiments that aren't enabled on the current platform.  We
  // don't remove these from prefs else syncing to a platform with a different
  // set of experiments would be lossy.
  std::set<std::string> platform_experiments;
  int current_platform = GetCurrentPlatform();
  for (size_t i = 0; i < num_experiments; ++i) {
    if (experiments[i].supported_platforms & current_platform)
      AddInternalName(experiments[i], &platform_experiments);
#if defined(OS_CHROMEOS)
    if (experiments[i].supported_platforms & kOsCrOSOwnerOnly)
      AddInternalName(experiments[i], &platform_experiments);
#endif
  }

  std::set<std::string> new_enabled_experiments;
  std::set_intersection(
      platform_experiments.begin(), platform_experiments.end(),
      result->begin(), result->end(),
      std::inserter(new_enabled_experiments, new_enabled_experiments.begin()));

  result->swap(new_enabled_experiments);
}

// Returns true if none of this entry's options have been enabled.
bool IsDefaultValue(
    const Experiment& experiment,
    const std::set<std::string>& enabled_experiments) {
  switch (experiment.type) {
    case Experiment::SINGLE_VALUE:
    case Experiment::SINGLE_DISABLE_VALUE:
      return enabled_experiments.count(experiment.internal_name) == 0;
    case Experiment::MULTI_VALUE:
    case Experiment::ENABLE_DISABLE_VALUE:
      for (int i = 0; i < experiment.num_choices; ++i) {
        if (enabled_experiments.count(experiment.NameForChoice(i)) > 0)
          return false;
      }
      return true;
  }
  NOTREACHED();
  return true;
}

// Returns the Value representing the choice data in the specified experiment.
base::Value* CreateChoiceData(const Experiment& experiment,
                        const std::set<std::string>& enabled_experiments) {
  DCHECK(experiment.type == Experiment::MULTI_VALUE ||
         experiment.type == Experiment::ENABLE_DISABLE_VALUE);
  base::ListValue* result = new base::ListValue;
  for (int i = 0; i < experiment.num_choices; ++i) {
    base::DictionaryValue* value = new base::DictionaryValue;
    const std::string name = experiment.NameForChoice(i);
    value->SetString("internal_name", name);
    value->SetString("description", experiment.DescriptionForChoice(i));
    value->SetBoolean("selected", enabled_experiments.count(name) > 0);
    result->Append(value);
  }
  return result;
}

}  // namespace

std::string Experiment::NameForChoice(int index) const {
  DCHECK(type == Experiment::MULTI_VALUE ||
         type == Experiment::ENABLE_DISABLE_VALUE);
  DCHECK_LT(index, num_choices);
  return std::string(internal_name) + testing::kMultiSeparator +
         base::IntToString(index);
}

base::string16 Experiment::DescriptionForChoice(int index) const {
  DCHECK(type == Experiment::MULTI_VALUE ||
         type == Experiment::ENABLE_DISABLE_VALUE);
  DCHECK_LT(index, num_choices);
  int description_id;
  if (type == Experiment::ENABLE_DISABLE_VALUE) {
    const int kEnableDisableDescriptionIds[] = {
      IDS_GENERIC_EXPERIMENT_CHOICE_DEFAULT,
      IDS_GENERIC_EXPERIMENT_CHOICE_ENABLED,
      IDS_GENERIC_EXPERIMENT_CHOICE_DISABLED,
    };
    description_id = kEnableDisableDescriptionIds[index];
  } else {
    description_id = choices[index].description_id;
  }
  return l10n_util::GetStringUTF16(description_id);
}

void ConvertFlagsToSwitches(
    const scoped_refptr<FlagsJsonStorage>& flags_storage,
    base::CommandLine* command_line) {
  FlagsState::GetInstance()->ConvertFlagsToSwitches(flags_storage,
                                                    command_line);
}

bool AreSwitchesIdenticalToCurrentCommandLine(
    const base::CommandLine& new_cmdline,
    const base::CommandLine& active_cmdline) {
  std::set<base::CommandLine::StringType> new_flags =
      ExtractFlagsFromCommandLine(new_cmdline);
  std::set<base::CommandLine::StringType> active_flags =
      ExtractFlagsFromCommandLine(active_cmdline);

  // Needed because std::equal doesn't check if the 2nd set is empty.
  if (new_flags.size() != active_flags.size())
    return false;

  return std::equal(new_flags.begin(), new_flags.end(), active_flags.begin());
}

void GetFlagFeatureEntries(
    const scoped_refptr<FlagsJsonStorage>& flags_storage,
    FlagAccess access,
    base::ListValue* supported_experiments,
    base::ListValue* unsupported_experiments) {
  std::set<std::string> enabled_experiments;
  GetSanitizedEnabledFlags(flags_storage, &enabled_experiments);

  int current_platform = GetCurrentPlatform();

  for (size_t i = 0; i < num_experiments; ++i) {
    const Experiment& experiment = experiments[i];

    base::DictionaryValue* data = new base::DictionaryValue();
    data->SetString("internal_name", experiment.internal_name);
    data->SetString("name",
                    l10n_util::GetStringUTF16(experiment.visible_name_id));
    data->SetString("description",
                    l10n_util::GetStringUTF16(
                        experiment.visible_description_id));

    base::ListValue* supported_platforms = new base::ListValue();
    AddOsStrings(experiment.supported_platforms, supported_platforms);
    data->Set("supported_platforms", supported_platforms);

    bool is_default_value = IsDefaultValue(experiment, enabled_experiments);
    switch (experiment.type) {
      case Experiment::SINGLE_VALUE:
      case Experiment::SINGLE_DISABLE_VALUE:
        data->SetBoolean(
            "enabled",
            (!is_default_value &&
             experiment.type == Experiment::SINGLE_VALUE) ||
            (is_default_value &&
             experiment.type == Experiment::SINGLE_DISABLE_VALUE));
        break;
      case Experiment::MULTI_VALUE:
      case Experiment::ENABLE_DISABLE_VALUE:
        data->Set("choices", CreateChoiceData(experiment, enabled_experiments));
        break;
      default:
        NOTREACHED();
    }

    bool supported = (experiment.supported_platforms & current_platform) != 0;
#if defined(OS_CHROMEOS)
    if (access == kOwnerAccessToFlags &&
        (experiment.supported_platforms & kOsCrOSOwnerOnly) != 0) {
      supported = true;
    }
#endif
    if (supported)
      supported_experiments->Append(data);
    else
      unsupported_experiments->Append(data);
  }
}

bool IsRestartNeededToCommitChanges() {
  return FlagsState::GetInstance()->IsRestartNeededToCommitChanges();
}

void SetFeatureEntryEnabled(
    const scoped_refptr<FlagsJsonStorage>& flags_storage,
    const std::string& internal_name,
    bool enable) {
  FlagsState::GetInstance()->SetFeatureEntryEnabled(flags_storage,
                                                  internal_name, enable);
}

void RemoveFlagsSwitches(
    std::map<std::string, base::CommandLine::StringType>* switch_list) {
  FlagsState::GetInstance()->RemoveFlagsSwitches(switch_list);
}

void ResetAllFlags(const scoped_refptr<FlagsJsonStorage>& flags_storage) {
  FlagsState::GetInstance()->ResetAllFlags(flags_storage);
}

int GetCurrentPlatform() {
#if defined(OS_MACOSX)
  return kOsMac;
#elif defined(OS_WIN)
  return kOsWin;
#elif defined(OS_CHROMEOS)  // Needs to be before the OS_LINUX check.
  return kOsCrOS;
#elif defined(OS_LINUX) || defined(OS_OPENBSD)
  return kOsLinux;
#elif defined(OS_ANDROID)
  return kOsAndroid;
#else
#error Unknown platform
#endif
}

void RecordUMAStatistics(
    const scoped_refptr<FlagsJsonStorage>& flags_storage) {
  std::set<std::string> flags = flags_storage->GetFlags();
  for (std::set<std::string>::iterator it = flags.begin(); it != flags.end();
       ++it) {
    std::string action("AboutFlags_");
    action += *it;
    content::RecordComputedAction(action);
  }
  // Since flag metrics are recorded every startup, add a tick so that the
  // stats can be made meaningful.
  if (flags.size())
    content::RecordAction(UserMetricsAction("AboutFlags_StartupTick"));
  content::RecordAction(UserMetricsAction("StartupTick"));
}

//////////////////////////////////////////////////////////////////////////////
// FlagsState implementation.

namespace {

typedef std::map<std::string, std::pair<std::string, std::string> >
    NameToSwitchAndValueMap;

void SetFlagToSwitchMapping(const std::string& key,
                            const std::string& switch_name,
                            const std::string& switch_value,
                            NameToSwitchAndValueMap* name_to_switch_map) {
  DCHECK(name_to_switch_map->end() == name_to_switch_map->find(key));
  (*name_to_switch_map)[key] = std::make_pair(switch_name, switch_value);
}

void FlagsState::ConvertFlagsToSwitches(
    const scoped_refptr<FlagsJsonStorage>& flags_storage,
    base::CommandLine* command_line) {
  std::set<std::string> enabled_experiments;

  GetSanitizedEnabledFlagsForCurrentPlatform(flags_storage,
                                             &enabled_experiments);

  NameToSwitchAndValueMap name_to_switch_map;
  for (size_t i = 0; i < num_experiments; ++i) {
    const Experiment& e = experiments[i];
    if (e.type == Experiment::SINGLE_VALUE ||
        e.type == Experiment::SINGLE_DISABLE_VALUE) {
      SetFlagToSwitchMapping(e.internal_name, e.command_line_switch,
                             e.command_line_value, &name_to_switch_map);
    } else if (e.type == Experiment::MULTI_VALUE) {
      for (int j = 0; j < e.num_choices; ++j) {
        SetFlagToSwitchMapping(e.NameForChoice(j),
                               e.choices[j].command_line_switch,
                               e.choices[j].command_line_value,
                               &name_to_switch_map);
      }
    } else {
      DCHECK_EQ(e.type, Experiment::ENABLE_DISABLE_VALUE);
      SetFlagToSwitchMapping(e.NameForChoice(0), std::string(), std::string(),
                             &name_to_switch_map);
      SetFlagToSwitchMapping(e.NameForChoice(1), e.command_line_switch,
                             e.command_line_value, &name_to_switch_map);
      SetFlagToSwitchMapping(e.NameForChoice(2), e.disable_command_line_switch,
                             e.disable_command_line_value, &name_to_switch_map);
    }
  }

  command_line->AppendSwitch(switches::kFlagSwitchesBegin);
  flags_switches_.insert(
      std::pair<std::string, std::string>(switches::kFlagSwitchesBegin,
                                          std::string()));
  for (std::set<std::string>::iterator it = enabled_experiments.begin();
       it != enabled_experiments.end();
       ++it) {
    const std::string& experiment_name = *it;
    NameToSwitchAndValueMap::const_iterator name_to_switch_it =
        name_to_switch_map.find(experiment_name);
    if (name_to_switch_it == name_to_switch_map.end()) {
      NOTREACHED();
      continue;
    }

    const std::pair<std::string, std::string>&
        switch_and_value_pair = name_to_switch_it->second;

    CHECK(!switch_and_value_pair.first.empty());
    command_line->AppendSwitchASCII(switch_and_value_pair.first,
                                    switch_and_value_pair.second);
    flags_switches_[switch_and_value_pair.first] = switch_and_value_pair.second;
  }
  command_line->AppendSwitch(switches::kFlagSwitchesEnd);
  flags_switches_.insert(
      std::pair<std::string, std::string>(switches::kFlagSwitchesEnd,
                                          std::string()));
}

bool FlagsState::IsRestartNeededToCommitChanges() {
  return needs_restart_;
}

void FlagsState::SetFeatureEntryEnabled(
    const scoped_refptr<FlagsJsonStorage>& flags_storage,
    const std::string& internal_name,
    bool enable) {
  size_t at_index = internal_name.find(testing::kMultiSeparator);
  if (at_index != std::string::npos) {
    DCHECK(enable);
    // We're being asked to enable a multi-choice experiment. Disable the
    // currently selected choice.
    DCHECK_NE(at_index, 0u);
    const std::string experiment_name = internal_name.substr(0, at_index);
    SetFeatureEntryEnabled(flags_storage, experiment_name, false);

    // And enable the new choice, if it is not the default first choice.
    if (internal_name != experiment_name + "@0") {
      std::set<std::string> enabled_experiments;
      GetSanitizedEnabledFlags(flags_storage, &enabled_experiments);
      needs_restart_ |= enabled_experiments.insert(internal_name).second;
      flags_storage->SetFlags(enabled_experiments);
    }
    return;
  }

  std::set<std::string> enabled_experiments;
  GetSanitizedEnabledFlags(flags_storage, &enabled_experiments);

  const Experiment* e = NULL;
  for (size_t i = 0; i < num_experiments; ++i) {
    if (experiments[i].internal_name == internal_name) {
      e = experiments + i;
      break;
    }
  }
  DCHECK(e);

  if (e->type == Experiment::SINGLE_VALUE) {
    if (enable)
      needs_restart_ |= enabled_experiments.insert(internal_name).second;
    else
      needs_restart_ |= (enabled_experiments.erase(internal_name) > 0);
  } else if (e->type == Experiment::SINGLE_DISABLE_VALUE) {
    if (!enable)
      needs_restart_ |= enabled_experiments.insert(internal_name).second;
    else
      needs_restart_ |= (enabled_experiments.erase(internal_name) > 0);
  } else {
    if (enable) {
      // Enable the first choice.
      needs_restart_ |= enabled_experiments.insert(e->NameForChoice(0)).second;
    } else {
      // Find the currently enabled choice and disable it.
      for (int i = 0; i < e->num_choices; ++i) {
        std::string choice_name = e->NameForChoice(i);
        if (enabled_experiments.find(choice_name) !=
            enabled_experiments.end()) {
          needs_restart_ = true;
          enabled_experiments.erase(choice_name);
          // Continue on just in case there's a bug and more than one
          // experiment for this choice was enabled.
        }
      }
    }
  }

  flags_storage->SetFlags(enabled_experiments);
}

void FlagsState::RemoveFlagsSwitches(
    std::map<std::string, base::CommandLine::StringType>* switch_list) {
  for (std::map<std::string, std::string>::const_iterator
           it = flags_switches_.begin(); it != flags_switches_.end(); ++it) {
    switch_list->erase(it->first);
  }
}

void FlagsState::ResetAllFlags(
    const scoped_refptr<FlagsJsonStorage>& flags_storage) {
  needs_restart_ = true;

  std::set<std::string> no_experiments;
  flags_storage->SetFlags(no_experiments);
}

void FlagsState::reset() {
  needs_restart_ = false;
  flags_switches_.clear();
}

}  // namespace

namespace testing {

// WARNING: '@' is also used in the html file. If you update this constant you
// also need to update the html file.
const char kMultiSeparator[] = "@";

void ClearState() {
  FlagsState::GetInstance()->reset();
}

void SetExperiments(const Experiment* e, size_t count) {
  if (!e) {
    experiments = kExperiments;
    num_experiments = arraysize(kExperiments);
  } else {
    experiments = e;
    num_experiments = count;
  }
}

const Experiment* GetExperiments(size_t* count) {
  *count = num_experiments;
  return experiments;
}

}  // namespace testing

}  // namespace about_flags
