// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/fraud_protection_ui/fraud_warning_page.h"

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/ui_base_export.h"

#include "common/fraud_protection/fraud_protection_service.h"
#include "common/fraud_protection_ui/fraud_protection_delegate.h"
#include "opera_common/grit/opera_common_resources.h"
#include "opera_common/grit/product_free_common_strings.h"
#include "opera_common/grit/product_related_common_strings.h"

#if defined(OPERA_DESKTOP)
#include "desktop/common/brand/brand_info.h"
#endif

namespace opera {

namespace {
static const char* const kProceedCommand = "proceed";
static const char* const kTakeMeBackCommand = "takeMeBack";
static const char* const kShowAdvisoryPage = "showAdvisoryPage";

std::string GetDefaultHomepageUrl() {
#if defined(OPERA_DESKTOP)
  return brand_info::Url(brand_info::kUrlHomepage);
#else
  return "http://www.opera.com/";
#endif
}
}  // namespace

FraudWarningPage::FraudWarningPage(content::WebContents* web_contents,
                                   const FraudUrlRating& rating,
                                   FraudProtectionDelegate* delegate)
    : needs_to_reload_on_proceeding_(false),
      web_contents_(web_contents),
      rating_(rating),
      delegate_(delegate) {
  page_ = content::InterstitialPage::Create(web_contents, false, GURL(), this);
  advisory_homepage_ = rating_.advisory_homepage();
  if (advisory_homepage_.empty()) {
    advisory_homepage_ = GetDefaultHomepageUrl();
  }
}

FraudWarningPage::~FraudWarningPage() {
}

std::string FraudWarningPage::GetHTMLContents() {
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  std::string html = rb.GetRawDataResource(IDR_FRAUD_WARNING_HTML).as_string();

  base::DictionaryValue strings;

  switch (rating_.rating()) {
    case FraudUrlRating::RATING_URL_PHISHING:
      strings.SetString("title", l10n_util::GetStringUTF16(IDS_PHISHING_WARNING_TITLE));
      strings.SetString("header",
                        l10n_util::GetStringUTF16(IDS_PHISHING_WARNING_HEADER));
      strings.SetString("summary",
                        l10n_util::GetStringUTF16(IDS_PHISHING_WARNING_SUMMARY));
      strings.SetString("suggestions",
                        l10n_util::GetStringUTF16(IDS_PHISHING_WARNING_SUGGESTION));
      break;

    case FraudUrlRating::RATING_URL_MALWARE:
      strings.SetString("title", l10n_util::GetStringUTF16(IDS_MALWARE_WARNING_TITLE));
      strings.SetString("header",
                        l10n_util::GetStringUTF16(IDS_MALWARE_WARNING_HEADER));
      strings.SetString("summary",
                        l10n_util::GetStringUTF16(IDS_MALWARE_WARNING_SUMMARY));
      strings.SetString("suggestions",
                        l10n_util::GetStringUTF16(IDS_MALWARE_WARNING_SUGGESTION));
      break;

    default:
      NOTREACHED();
  }

  strings.SetString("advisoryName", base::UTF8ToUTF16(rating_.advisory_text()));
  strings.SetString("advisoryHomepage", base::UTF8ToUTF16(advisory_homepage_));
  strings.SetString("advisoryImage",
                    base::ASCIIToUTF16(rating_.advisory_logo_url()));

  strings.SetString("advisoryText", l10n_util::GetStringFUTF8(
      IDS_FRAUDWARNING_ADVISORY_TEXT,
      base::ASCIIToUTF16(rating_.advisory_homepage()),
      base::UTF8ToUTF16(rating_.advisory_text())));

  strings.SetString("takeMeBackButtonTitle",
                    l10n_util::GetStringUTF16(IDS_FRAUDWARNING_BACK_BUTTON));
  strings.SetString("proceedLinkText",
                    l10n_util::GetStringUTF16(IDS_FRAUDWARNING_PROCEED_LINK));
  strings.SetString("whyBlockedLinkText",
                    l10n_util::GetStringUTF16(IDS_FRAUDWARNING_WHY_LINK));
  strings.SetString("moreInfoText",
                    l10n_util::GetStringUTF16(IDS_FRAUDWARNING_MORE_INFO_TEXT));

  return webui::GetTemplatesHtml(html, &strings, "template_root");
}

void FraudWarningPage::CommandReceived(const std::string& cmd) {
  std::string command(cmd);
  if (command.length() > 1 && command[0] == '"') {
    command = command.substr(1, command.length() - 2);
  }

  if (command == kProceedCommand) {
    FraudProtectionService* service =
        delegate_->GetFraudProtectionService(web_contents_);
    if (service)
      service->BypassUrlRating(rating_);
    page_->Proceed();
    if (needs_to_reload_on_proceeding_)
      web_contents_->GetController().Reload(false);
  } else if (command == kTakeMeBackCommand) {
    if (web_contents_->GetController().CanGoBack()) {
      web_contents_->GetController().GoBack();
    } else {
      delegate_->OnTakeMeBackToStart(web_contents_);
    }
  } else if (command == kShowAdvisoryPage) {
    GURL report_url(advisory_homepage_);
    content::OpenURLParams params(
        report_url, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
        ui::PAGE_TRANSITION_LINK, false);
    web_contents_->OpenURL(params);
  }

  FOR_EACH_OBSERVER(Observer, observers_,
                    OnFraudWarningDismissed(web_contents_));
}

void FraudWarningPage::Show() {
  page_->Show();
  page_->Focus();
  FOR_EACH_OBSERVER(Observer, observers_, OnFraudWarningShown(web_contents_));
}

}  // namespace opera
