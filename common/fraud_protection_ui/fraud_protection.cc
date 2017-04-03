// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/fraud_protection_ui/fraud_protection.h"

#include <string>

#include "base/bind.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/frame_navigate_params.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

#include "common/fraud_protection/fraud_url_rating.h"
#include "common/fraud_protection/fraud_protection_service.h"
#include "common/fraud_protection_ui/fraud_protection_delegate.h"
#include "common/fraud_protection_ui/fraud_warning_page.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(opera::FraudProtection);

namespace opera {

FraudProtection::FraudProtection(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      web_contents_(web_contents),
      delegate_(FraudProtection::CreateDelegate(this)),
      navigation_entry_commited_(false),
      provisional_load_failed(false),
      navigation_blocking_state_(NO_BLOCKING),
      weakptr_factory_(this) {}

FraudProtection::~FraudProtection() {
}

void FraudProtection::DidStartProvisionalLoadForFrame(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    bool is_error_page,
    bool is_iframe_srcdoc) {
  DCHECK(delegate_.get());

  if (!render_frame_host->GetParent() && !provisional_load_failed) {
    navigation_entry_commited_ = false;
    navigation_blocking_state_ = NO_BLOCKING;
    deferred_warning_page_.reset(NULL);
    CheckUrl(validated_url);
  }
}

void FraudProtection::DidFailProvisionalLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code,
    const base::string16& error_description,
    bool was_ignored_by_handler) {
  provisional_load_failed = true;
}

void FraudProtection::DidNavigateMainFrame(
    const content::LoadCommittedDetails& details,
    const content::FrameNavigateParams& params) {
  navigation_entry_commited_ = true;
  if (deferred_warning_page_) {
    DCHECK(!web_contents_->GetController().GetPendingEntry());
    ShowFraudWarningPage(*deferred_warning_page_);
    deferred_warning_page_.reset(NULL);
  } else if (!provisional_load_failed) {
    navigation_blocking_state_ = NO_BLOCKING;
  }
  provisional_load_failed = false;
}

void FraudProtection::DidGetRedirectForResourceRequest(
    const content::ResourceRedirectDetails& details) {
  if (details.resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    CheckUrl(details.new_url);
}

void FraudProtection::CheckUrl(const GURL& url) {
  FraudProtectionService* service =
      delegate_->GetFraudProtectionService(web_contents_);

  if (service && service->IsEnabled()) {
    weakptr_factory_.InvalidateWeakPtrs();
    delegate_->NotifyAboutNavigationBlocked(web_contents_, true);
    service->GetUrlRating(url,
                          std::string(),
                          base::Bind(&FraudProtection::UrlChecked,
                                     weakptr_factory_.GetWeakPtr()));
  }
}

void FraudProtection::UrlChecked(const FraudUrlRating& rating) {
  if (!rating.IsBypassed() &&
      (rating.rating() == FraudUrlRating::RATING_URL_PHISHING ||
       rating.rating() == FraudUrlRating::RATING_URL_MALWARE)) {
    ShowFraudWarningPage(rating);
    if (navigation_blocking_state_ == NEEDS_UNBLOCKING_TO_FINISH)
      OnNavigationBlocked();  // It'll unblock now.
  } else {
    // No need to listen to this any longer.
    delegate_->NotifyAboutNavigationBlocked(web_contents_, false);
  }
}

void FraudProtection::ShowFraudWarningPage(const FraudUrlRating& rating) {
  if (!navigation_entry_commited_ && !provisional_load_failed) {
    // Fraud rating is available but we need to wait for pending entry to
    // be commited before we can show a warning page. If we show fraud
    // warning page now, it will be removed when loaded page entry gets
    // commited.
    deferred_warning_page_.reset(new FraudUrlRating(rating));
    return;
  }

  // Ownership taken by WebContents.
  FraudWarningPage* page =
      delegate_->CreateFraudWarningPage(web_contents_, rating);
  // Fraud protection always outlives FraudWarningPage. No nee to remove this
  // observer.
  page->AddObserver(this);
  page->Show();
  if (navigation_blocking_state_ == FORCED_UNBLOCKING) {
    page->SetNeedsToReloadOnProceed();
    navigation_blocking_state_ = NO_BLOCKING;
  }
}

void FraudProtection::OnFraudWarningShown(content::WebContents* web_contents) {
  DCHECK(web_contents == web_contents_);
  FOR_EACH_OBSERVER(Observer, observers_, OnFraudWarningShown(web_contents_));
}

void FraudProtection::OnFraudWarningDismissed(
    content::WebContents* web_contents) {
  DCHECK(web_contents == web_contents_);
  FOR_EACH_OBSERVER(Observer, observers_,
                    OnFraudWarningDismissed(web_contents_));
}

void FraudProtection::OnNavigationBlocked() {
  // If check is done and were facing phishing/fraud here.
  if (deferred_warning_page_) {
    delegate_->ForceUnblockingNavigation(web_contents_);
    navigation_blocking_state_ = FORCED_UNBLOCKING;
  } else {
    navigation_blocking_state_ = NEEDS_UNBLOCKING_TO_FINISH;
  }
}

}  // namespace opera
