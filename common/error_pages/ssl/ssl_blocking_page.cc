// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA.
// @copied-from chromium/src/chrome/browser/ssl/ssl_blocking_page.cc
// @final-synchronized cec058ea5cd04c3f65b2668a4cabc81672682da9

#include "common/error_pages/ssl/ssl_blocking_page.h"

#include "base/i18n/rtl.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/ssl_status.h"
#include "net/base/url_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

#include "components/ssl_errors/error_info.h"
#include "opera_common/grit/opera_common_resources.h"
#include "opera_common/grit/product_free_common_strings.h"
#include "opera_common/grit/product_related_common_strings.h"

using base::ASCIIToUTF16;
using content::InterstitialPage;
using content::NavigationEntry;

namespace {

// Events for UMA. Do not reorder or change!
enum SSLBlockingPageEvent {
  SHOW_ALL,
  SHOW_OVERRIDABLE,
  PROCEED_OVERRIDABLE,
  PROCEED_NAME,
  PROCEED_DATE,
  PROCEED_AUTHORITY,
  DONT_PROCEED_OVERRIDABLE,
  DONT_PROCEED_NAME,
  DONT_PROCEED_DATE,
  DONT_PROCEED_AUTHORITY,
  SHOW_INTERNAL_HOSTNAME,
  PROCEED_INTERNAL_HOSTNAME,
  PROCEED_MANUAL_NONOVERRIDABLE,
  UNUSED_BLOCKING_PAGE_EVENT,
};

// Events for UMA. Do not reorder or change!
enum SSLExpirationAndDecision {
  EXPIRED_AND_PROCEED,
  EXPIRED_AND_DO_NOT_PROCEED,
  NOT_EXPIRED_AND_PROCEED,
  NOT_EXPIRED_AND_DO_NOT_PROCEED,
  END_OF_SSL_EXPIRATION_AND_DECISION,
};

void RecordSSLBlockingPageEventStats(SSLBlockingPageEvent event) {
  UMA_HISTOGRAM_ENUMERATION("interstitial.ssl",
                            event,
                            UNUSED_BLOCKING_PAGE_EVENT);
}

void RecordSSLExpirationPageEventState(bool expired_but_previously_allowed,
                                       bool proceed,
                                       bool overridable) {
  SSLExpirationAndDecision event;
  if (expired_but_previously_allowed && proceed)
    event = EXPIRED_AND_PROCEED;
  else if (expired_but_previously_allowed && !proceed)
    event = EXPIRED_AND_DO_NOT_PROCEED;
  else if (!expired_but_previously_allowed && proceed)
    event = NOT_EXPIRED_AND_PROCEED;
  else
    event = NOT_EXPIRED_AND_DO_NOT_PROCEED;

  if (overridable) {
    UMA_HISTOGRAM_ENUMERATION(
        "interstitial.ssl.expiration_and_decision.overridable",
        event,
        END_OF_SSL_EXPIRATION_AND_DECISION);
  } else {
    UMA_HISTOGRAM_ENUMERATION(
        "interstitial.ssl.expiration_and_decision.nonoverridable",
        event,
        END_OF_SSL_EXPIRATION_AND_DECISION);
  }
}

void RecordSSLBlockingPageDetailedStats(
    bool proceed,
    int cert_error,
    bool overridable,
    bool internal,
    bool expired_but_previously_allowed) {
  UMA_HISTOGRAM_ENUMERATION("interstitial.ssl_error_type",
     ssl_errors::ErrorInfo::NetErrorToErrorType(cert_error), ssl_errors::ErrorInfo::END_OF_ENUM);
  RecordSSLExpirationPageEventState(
      expired_but_previously_allowed, proceed, overridable);
  if (!overridable) {
    if (proceed) {
      RecordSSLBlockingPageEventStats(PROCEED_MANUAL_NONOVERRIDABLE);
    }
    // Overridable is false if the user didn't have any option except to turn
    // back. If that's the case, don't record some of the metrics.
    return;
  }
  if (proceed) {
    RecordSSLBlockingPageEventStats(PROCEED_OVERRIDABLE);
    if (internal)
      RecordSSLBlockingPageEventStats(PROCEED_INTERNAL_HOSTNAME);
  } else if (!proceed) {
    RecordSSLBlockingPageEventStats(DONT_PROCEED_OVERRIDABLE);
  }
  ssl_errors::ErrorInfo::ErrorType type = ssl_errors::ErrorInfo::NetErrorToErrorType(cert_error);
  switch (type) {
    case ssl_errors::ErrorInfo::CERT_COMMON_NAME_INVALID: {
      if (proceed)
        RecordSSLBlockingPageEventStats(PROCEED_NAME);
      else
        RecordSSLBlockingPageEventStats(DONT_PROCEED_NAME);
      break;
    }
    case ssl_errors::ErrorInfo::CERT_DATE_INVALID: {
      if (proceed)
        RecordSSLBlockingPageEventStats(PROCEED_DATE);
      else
        RecordSSLBlockingPageEventStats(DONT_PROCEED_DATE);
      break;
    }
    case ssl_errors::ErrorInfo::CERT_AUTHORITY_INVALID: {
      if (proceed)
        RecordSSLBlockingPageEventStats(PROCEED_AUTHORITY);
      else
        RecordSSLBlockingPageEventStats(DONT_PROCEED_AUTHORITY);
      break;
    }
    default: {
      break;
    }
  }
}

/*
void LaunchDateAndTimeSettings() { ... }
 * In the chromium file ssl_blocking_page.cc, this function enables the user to
 * launch the date and time preferences (for the case where the system clock
 * is causing the SSL error). On our interstitial page, we have no such option,
 * so this function is omitted.

bool IsErrorDueToBadClock(const base::Time& now, int error) { ... }
 * Function to determine if the system clock is way off. Not used by Opera.
 */

}  // namespace

// Note that we always create a navigation entry with SSL errors.
// No error happening loading a sub-resource triggers an interstitial so far.
SSLBlockingPage::SSLBlockingPage(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    const std::string& app_locale,
    int options_mask,
    const base::Callback<void(content::CertificateRequestResultType)>& callback)
    : callback_(callback),
      web_contents_(web_contents),
      cert_error_(cert_error),
      ssl_info_(ssl_info),
      request_url_(request_url),
      app_locale_(app_locale),
      overridable_(options_mask & OVERRIDABLE &&
                   !(options_mask & STRICT_ENFORCEMENT)),
      strict_enforcement_((options_mask & STRICT_ENFORCEMENT) != 0),
      interstitial_page_(NULL),
      internal_(false),
      expired_but_previously_allowed_(
          (options_mask & EXPIRED_BUT_PREVIOUSLY_ALLOWED) != 0) {
  // For UMA stats.
  if (net::IsHostnameNonUnique(request_url_.HostNoBrackets()))
    internal_ = true;
  RecordSSLBlockingPageEventStats(SHOW_ALL);
  if (overridable_) {
    RecordSSLBlockingPageEventStats(SHOW_OVERRIDABLE);
    if (internal_)
      RecordSSLBlockingPageEventStats(SHOW_INTERNAL_HOSTNAME);
  }

/*
  ssl_error_classification_.reset(new SSLErrorClassification( ...
 * Opera doesn't log captive portal status.
 */

  // Creating an interstitial without showing (e.g. from chrome://interstitials)
  // it leaks memory, so don't create it here.
}

SSLBlockingPage::~SSLBlockingPage() {
/*
  ssl_errors::ErrorInfo::ErrorType type =
      ssl_errors::ErrorInfo::NetErrorToErrorType(cert_error_);
  switch (type) { ... }
 * Opera doesn't log captive portal status.
 */
  if (!callback_.is_null()) {
    RecordSSLBlockingPageDetailedStats(false,
                                       cert_error_,
                                       overridable_,
                                       internal_,
                                       expired_but_previously_allowed_);
    // The page is closed without the user having chosen what to do, default to
    // deny.
    NotifyDenyCertificate();
  }
}

void SSLBlockingPage::Show() {
  DCHECK(!interstitial_page_);
  interstitial_page_ = InterstitialPage::Create(
      web_contents_, true, request_url_, this);
  interstitial_page_->Show();
}

std::string SSLBlockingPage::GetHTMLContents() {
  base::DictionaryValue load_time_data;
  base::string16 url(ASCIIToUTF16(request_url_.host()));
  if (base::i18n::IsRTL())
    base::i18n::WrapStringWithLTRFormatting(&url);
  webui::SetLoadTimeDataDefaults(app_locale_, &load_time_data);

  bool bad_clock = false;

  if (bad_clock) {
    /* Opera has no check for internal system clock */
  } else {
    /* Opera's blocking page has the following elements:
     * - Tab title
     * - Image/Icon
     * - Heading
     * - Paragraph with description of error
     * - Paragraph with advice not to proceed
     * - Button to go back to safety
     * - Button for proceeding or reloading
     */

    load_time_data.SetString(
        "tabTitle", l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_TITLE));
    load_time_data.SetString(
        "heading", l10n_util::GetStringUTF16(IDS_SSL_ERROR_PAGE_TITLE));
    load_time_data.SetBoolean("overridable", overridable_);
    load_time_data.SetString(
        "backButton", l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_EXIT));

    ssl_errors::ErrorInfo error_info =
        ssl_errors::ErrorInfo::CreateError(
            ssl_errors::ErrorInfo::NetErrorToErrorType(cert_error_),
            ssl_info_.cert.get(),
            request_url_);
    load_time_data.SetString("explanationParagraph", error_info.details());

    if (overridable_) {
      load_time_data.SetString("shouldNotProceed",
          l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_SHOULD_NOT_PROCEED));
      load_time_data.SetString(
          "proceedButton",
          l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_PROCEED));
    } else {
      if (strict_enforcement_) {
        load_time_data.SetString("shouldNotProceed",
                          l10n_util::GetStringUTF16(
                              IDS_SSL_ERROR_PAGE_CANNOT_PROCEED));
      } else {
        load_time_data.SetString("shouldNotProceed", "");
      }
      load_time_data.SetString(
          "proceedButton",
          l10n_util::GetStringUTF16(IDS_ERRORPAGES_BUTTON_RELOAD));
    }
  }
/*
  // Set debugging information at the bottom of the warning.
  ...
 * No debugging information is printed in Opera.
 */
  base::StringPiece html(
     ResourceBundle::GetSharedInstance().GetRawDataResource(
         IDR_SSL_ROAD_BLOCK_HTML));
  return webui::GetI18nTemplateHtml(html, &load_time_data);
}

void SSLBlockingPage::OverrideEntry(NavigationEntry* entry) {
  entry->GetSSL() = content::SSLStatus(
      content::SECURITY_STYLE_AUTHENTICATION_BROKEN, ssl_info_.cert, ssl_info_);
}

// This handles the commands sent from the interstitial JavaScript. They are
// defined in common/resources/webui/ssl_roadblock.js.
// DO NOT reorder or change this logic without also changing the JavaScript!
void SSLBlockingPage::CommandReceived(const std::string& command) {
  int cmd = 0;
  bool retval = base::StringToInt(command, &cmd);
  DCHECK(retval);
  switch (cmd) {
    case CMD_DONT_PROCEED: {
      interstitial_page_->DontProceed();
      break;
    }
    case CMD_PROCEED: {
      interstitial_page_->Proceed();
      break;
    }
    case CMD_RELOAD: {
      // The interstitial can't refresh itself.
      web_contents_->GetController().Reload(true);
      break;
    }
    default: {
      NOTREACHED();
    }
  }
}

void SSLBlockingPage::OnProceed() {
  RecordSSLBlockingPageDetailedStats(true,
                                     cert_error_,
                                     overridable_,
                                     internal_,
                                     expired_but_previously_allowed_);
  // Accepting the certificate resumes the loading of the page.
  NotifyAllowCertificate();
}

void SSLBlockingPage::OnDontProceed() {
  RecordSSLBlockingPageDetailedStats(false,
                                     cert_error_,
                                     overridable_,
                                     internal_,
                                     expired_but_previously_allowed_);
  NotifyDenyCertificate();
}

void SSLBlockingPage::NotifyDenyCertificate() {
  // It's possible that callback_ may not exist if the user clicks "Proceed"
  // followed by pressing the back button before the interstitial is hidden.
  // In that case the certificate will still be treated as allowed.
  if (callback_.is_null())
    return;

  callback_.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_CANCEL);
  callback_.Reset();
}

void SSLBlockingPage::NotifyAllowCertificate() {
  DCHECK(!callback_.is_null());

  callback_.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_CONTINUE);
  callback_.Reset();
}

/*
void SSLBlockingPage::SetExtraInfo(
    base::DictionaryValue* strings,
    const std::vector<base::string16>& extra_info) { ... }

 * Extra info is no longer used for interstitial pages.
 */

/*
void SSLBlockingPage::OnGotHistoryCount(HistoryService::Handle handle,
                                        bool success,
                                        int num_visits,
                                        base::Time first_visit) { ...}
*/
