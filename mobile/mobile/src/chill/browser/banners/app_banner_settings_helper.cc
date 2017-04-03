// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/banners/app_baner_settings_helper.cc
// @final-synchronized

#include "chill/browser/banners/app_banner_settings_helper.h"

#include <algorithm>
#include <string>

#include "base/command_line.h"
#include "base/values.h"
#include "base/memory/ptr_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/web_contents.h"
#include "net/base/escape.h"
#include "url/gurl.h"

#include "chill/browser/content_settings/host_content_settings_map_factory.h"
#include "chill/browser/opera_browser_context.h"
#include "chill/browser/banners/app_banner_data_fetcher.h"
#include "chill/common/opera_switches.h"

namespace {

// Max number of apps (including ServiceWorker based web apps) that a particular
// site may show a banner for.
const size_t kMaxAppsPerSite = 3;

// Oldest could show banner event we care about, in days.
const unsigned int kOldestCouldShowBannerEventInDays = 14;

// Total site engagements where a banner could have been shown before
// a banner will actually be triggered.
const double kTotalEngagementToTrigger = 2;

// Number of days that showing the banner will prevent it being seen again for.
const unsigned int kMinimumDaysBetweenBannerShows = 1;

// Dictionary keys to use for the events.
const char* kBannerEventKeys[] = {
    "couldShowBannerEvents",
    "didShowBannerEvent",
    "didBlockBannerEvent",
    "didAddToHomescreenEvent",
};

// Keys to use when storing BannerEvent structs.
const char kBannerTimeKey[] = "time";
const char kBannerEngagementKey[] = "engagement";

// Engagement weight assigned to direct and indirect navigations.
// By default, a direct navigation is a page visit via ui::PAGE_TRANSITION_TYPED
// or ui::PAGE_TRANSITION_GENERATED. These are weighted twice the engagement of
// all other navigations.
double kDirectNavigationEngagement = 1;
double kIndirectNavigationEnagagement = 1;

std::unique_ptr<base::DictionaryValue> GetOriginDict(
    HostContentSettingsMap* settings,
    const GURL& origin_url) {
  if (!settings)
    return base::WrapUnique(new base::DictionaryValue());

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(settings->GetWebsiteSetting(
          origin_url, origin_url, CONTENT_SETTINGS_TYPE_APP_BANNER,
          std::string(), NULL));
  if (!dict)
    return base::WrapUnique(new base::DictionaryValue());

  return dict;
}

base::DictionaryValue* GetAppDict(base::DictionaryValue* origin_dict,
                                  const std::string& key_name) {
  base::DictionaryValue* app_dict = nullptr;
  if (!origin_dict->GetDictionaryWithoutPathExpansion(key_name, &app_dict)) {
    // Don't allow more than kMaxAppsPerSite dictionaries.
    if (origin_dict->size() < kMaxAppsPerSite) {
      app_dict = new base::DictionaryValue();
      origin_dict->SetWithoutPathExpansion(key_name, base::WrapUnique(app_dict));
    }
  }

  return app_dict;
}

double GetEventEngagement(ui::PageTransition transition_type) {
  if (ui::PageTransitionCoreTypeIs(transition_type,
                                   ui::PAGE_TRANSITION_TYPED) ||
      ui::PageTransitionCoreTypeIs(transition_type,
                                   ui::PAGE_TRANSITION_GENERATED)) {
    return kDirectNavigationEngagement;
  } else {
    return kIndirectNavigationEnagagement;
  }
}

}  // namespace

namespace opera {

void AppBannerSettingsHelper::ClearBannerEvents() {
  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForForBrowserContext(
          OperaBrowserContext::GetDefaultBrowserContext());
  settings->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_APP_BANNER);
  settings->FlushLossyWebsiteSettings();
}

void AppBannerSettingsHelper::ClearHistoryForURLs(
    content::WebContents* web_contents,
    const std::set<GURL>& origin_urls) {
  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForForBrowserContext(
          web_contents->GetBrowserContext());
  for (const GURL& origin_url : origin_urls) {
    settings->SetWebsiteSettingDefaultScope(origin_url, GURL(),
                                            CONTENT_SETTINGS_TYPE_APP_BANNER,
                                            std::string(), nullptr);
    settings->FlushLossyWebsiteSettings();
  }
}

void AppBannerSettingsHelper::RecordBannerInstallEvent(
    content::WebContents* web_contents,
    const std::string& package_name_or_start_url) {
  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents, web_contents->GetURL(),
      package_name_or_start_url,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN,
      opera::AppBannerDataFetcher::GetCurrentTime());
}

void AppBannerSettingsHelper::RecordBannerBlockEvent(
    content::WebContents* web_contents,
    const std::string& package_name_or_start_url) {
  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents, web_contents->GetURL(),
      package_name_or_start_url,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_BLOCK,
      AppBannerDataFetcher::GetCurrentTime());
}

void AppBannerSettingsHelper::RecordBannerEvent(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    AppBannerEvent event,
    base::Time time) {
  DCHECK(event != APP_BANNER_EVENT_COULD_SHOW);

  if (web_contents->GetBrowserContext() ==
          OperaBrowserContext::GetPrivateBrowserContext() ||
      package_name_or_start_url.empty())
    return;

  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForForBrowserContext(
          web_contents->GetBrowserContext());
  std::unique_ptr<base::DictionaryValue> origin_dict =
      GetOriginDict(settings, origin_url);
  if (!origin_dict)
    return;

  base::DictionaryValue* app_dict =
      GetAppDict(origin_dict.get(), package_name_or_start_url);
  if (!app_dict)
    return;

  // Dates are stored in their raw form (i.e. not local dates) to be resilient
  // to time zone changes.
  std::string event_key(kBannerEventKeys[event]);
  app_dict->SetDouble(event_key, time.ToInternalValue());

  settings->SetWebsiteSettingDefaultScope(
      origin_url, GURL(), CONTENT_SETTINGS_TYPE_APP_BANNER, std::string(),
      std::move(origin_dict));

  // App banner content settings are lossy, meaning they will not cause the
  // prefs to become dirty. This is fine for most events, as if they are lost it
  // just means the user will have to engage a little bit more. However the
  // DID_ADD_TO_HOMESCREEN event should always be recorded to prevent
  // spamminess.
  if (event == APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN ||
      event == APP_BANNER_EVENT_DID_BLOCK) {
    settings->FlushLossyWebsiteSettings();
  }
}

void AppBannerSettingsHelper::RecordBannerCouldShowEvent(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    base::Time time,
    ui::PageTransition transition_type) {
  if (web_contents->GetBrowserContext() ==
          OperaBrowserContext::GetPrivateBrowserContext() ||
      package_name_or_start_url.empty())
    return;

  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForForBrowserContext(
          web_contents->GetBrowserContext());
  std::unique_ptr<base::DictionaryValue> origin_dict =
      GetOriginDict(settings, origin_url);
  if (!origin_dict)
    return;

  base::DictionaryValue* app_dict =
      GetAppDict(origin_dict.get(), package_name_or_start_url);
  if (!app_dict)
    return;

  std::string event_key(kBannerEventKeys[APP_BANNER_EVENT_COULD_SHOW]);
  double engagement = GetEventEngagement(transition_type);

  base::ListValue* could_show_list = nullptr;
  if (!app_dict->GetList(event_key, &could_show_list)) {
    could_show_list = new base::ListValue();
    app_dict->Set(event_key, base::WrapUnique(could_show_list));
  }

  // Trim any items that are older than we should care about. For comparisons
  // the times are converted to local dates.
  base::Time date = time.LocalMidnight();
  base::ListValue::iterator it = could_show_list->begin();
  while (it != could_show_list->end()) {
    if ((*it)->IsType(base::Value::TYPE_DICTIONARY)) {
      base::DictionaryValue* internal_value;
      double internal_date;
      (*it)->GetAsDictionary(&internal_value);

      if (internal_value->GetDouble(kBannerTimeKey, &internal_date)) {
        base::Time other_date =
            base::Time::FromInternalValue(internal_date).LocalMidnight();
        if (other_date == date) {
          double other_engagement = 0;
          if (internal_value->GetDouble(kBannerEngagementKey,
                                        &other_engagement) &&
              other_engagement >= engagement) {
            // This date has already been added, but with an equal or higher
            // engagement. Don't add the date again. If the conditional fails,
            // fall to the end of the loop where the existing entry is deleted.
            return;
          }
        } else {
          base::TimeDelta delta = date - other_date;
          if (delta <
              base::TimeDelta::FromDays(kOldestCouldShowBannerEventInDays)) {
            ++it;
            continue;
          }
        }
      }
    }

    // Either this date is older than we care about, or it isn't in the correct
    // format, or it is the same as the current date but with a lower
    // engagement, so remove it.
    it = could_show_list->Erase(it, nullptr);
  }

  // Dates are stored in their raw form (i.e. not local dates) to be resilient
  // to time zone changes.
  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  value->SetDouble(kBannerTimeKey, time.ToInternalValue());
  value->SetDouble(kBannerEngagementKey, engagement);
  could_show_list->Append(std::move(value));

  settings->SetWebsiteSettingDefaultScope(
      origin_url, GURL(), CONTENT_SETTINGS_TYPE_APP_BANNER, std::string(),
      std::move(origin_dict));
}

bool AppBannerSettingsHelper::IsAddedOrBlocked(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url) {
  // Ignore all checks if the flag to do so is set.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kBypassAppBannerEngagementChecks)) {
    return false;
  }

  base::Time added_time =
      GetSingleBannerEvent(web_contents, origin_url, package_name_or_start_url,
                           APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN);
  if (!added_time.is_null()) {
    return true;
  }

  base::Time blocked_time =
      GetSingleBannerEvent(web_contents, origin_url, package_name_or_start_url,
                           APP_BANNER_EVENT_DID_BLOCK);
  if (!blocked_time.is_null()) {
    return true;
  }

  return false;
}

bool AppBannerSettingsHelper::ShouldShowBanner(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    base::Time time) {
  // Ignore all checks if the flag to do so is set.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kBypassAppBannerEngagementChecks)) {
    return true;
  }

  base::Time shown_time =
      GetSingleBannerEvent(web_contents, origin_url, package_name_or_start_url,
                           APP_BANNER_EVENT_DID_SHOW);
  // Null times are in the distant past, so the delta between real times and
  // null events will always be greater than the limits.
  if (time - shown_time <
      base::TimeDelta::FromDays(kMinimumDaysBetweenBannerShows)) {
    return false;
  }

  std::vector<BannerEvent> could_show_events = GetCouldShowBannerEvents(
      web_contents, origin_url, package_name_or_start_url);

  // Return true if the total engagement of each applicable could show event
  // meets the trigger threshold.
  double total_engagement = 0;
  for (const auto& event : could_show_events)
    total_engagement += event.engagement;

  if (total_engagement < kTotalEngagementToTrigger) {
    return false;
  }

  return true;
}

bool AppBannerSettingsHelper::HasShownBanner(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url) {
  // Ignore all checks if the flag to do so is set.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kBypassAppBannerEngagementChecks)) {
    return false;
  }

  base::Time shown_time =
      GetSingleBannerEvent(web_contents, origin_url, package_name_or_start_url,
                           APP_BANNER_EVENT_DID_SHOW);
  return !shown_time.is_null();
}

std::vector<AppBannerSettingsHelper::BannerEvent>
AppBannerSettingsHelper::GetCouldShowBannerEvents(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url) {
  std::vector<BannerEvent> result;
  if (package_name_or_start_url.empty())
    return result;

  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForForBrowserContext(
          web_contents->GetBrowserContext());
  std::unique_ptr<base::DictionaryValue> origin_dict =
      GetOriginDict(settings, origin_url);

  if (!origin_dict)
    return result;

  base::DictionaryValue* app_dict =
      GetAppDict(origin_dict.get(), package_name_or_start_url);
  if (!app_dict)
    return result;

  std::string event_key(kBannerEventKeys[APP_BANNER_EVENT_COULD_SHOW]);
  base::ListValue* could_show_list = nullptr;
  if (!app_dict->GetList(event_key, &could_show_list))
    return result;

  for (const auto &value : *could_show_list) {
    if (value->IsType(base::Value::TYPE_DICTIONARY)) {
      base::DictionaryValue* internal_value;
      double internal_date = 0;
      value->GetAsDictionary(&internal_value);
      double engagement = 0;

      if (internal_value->GetDouble(kBannerTimeKey, &internal_date) &&
          internal_value->GetDouble(kBannerEngagementKey, &engagement)) {
        base::Time date = base::Time::FromInternalValue(internal_date);
        result.push_back({date, engagement});
      }
    }
  }

  return result;
}

base::Time AppBannerSettingsHelper::GetSingleBannerEvent(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    AppBannerEvent event) {
  DCHECK(event != APP_BANNER_EVENT_COULD_SHOW);
  DCHECK(event < APP_BANNER_EVENT_NUM_EVENTS);

  if (package_name_or_start_url.empty())
    return base::Time();

  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForForBrowserContext(
          web_contents->GetBrowserContext());
  std::unique_ptr<base::DictionaryValue> origin_dict =
      GetOriginDict(settings, origin_url);

  if (!origin_dict)
    return base::Time();

  base::DictionaryValue* app_dict =
      GetAppDict(origin_dict.get(), package_name_or_start_url);
  if (!app_dict)
    return base::Time();

  std::string event_key(kBannerEventKeys[event]);
  double internal_time;
  if (!app_dict->GetDouble(event_key, &internal_time))
    return base::Time();

  return base::Time::FromInternalValue(internal_time);
}

void AppBannerSettingsHelper::SetEngagementWeights(double direct_engagement,
                                                   double indirect_engagement) {
  kDirectNavigationEngagement = direct_engagement;
  kIndirectNavigationEnagagement = indirect_engagement;
}

}  // namespace opera
