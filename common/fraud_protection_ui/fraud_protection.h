// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FRAUD_PROTECTION_UI_FRAUD_PROTECTION_H_
#define COMMON_FRAUD_PROTECTION_UI_FRAUD_PROTECTION_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#include "common/fraud_protection_ui/fraud_warning_page.h"

namespace opera {

class FraudProtectionDelegate;
class FraudUrlRating;

class FraudProtection : public content::WebContentsObserver,
                        public content::WebContentsUserData<FraudProtection>,
                        public FraudWarningPage::Observer {
 public:
  class Observer {
   public:
    virtual void OnFraudWarningShown(content::WebContents*) = 0;
    virtual void OnFraudWarningDismissed(content::WebContents*) = 0;
  };

  ~FraudProtection() override;

  // WebContentsObserver implementation.
  void DidStartProvisionalLoadForFrame(
      content::RenderFrameHost* render_frame_host,
      const GURL& validated_url,
      bool is_error_page,
      bool is_iframe_srcdoc) override;

  void DidFailProvisionalLoad(
      content::RenderFrameHost* render_frame_host,
      const GURL& validated_url,
      int error_code,
      const base::string16& error_description,
      bool was_ignored_by_handler) override;

  void DidNavigateMainFrame(const content::LoadCommittedDetails& details,
                            const content::FrameNavigateParams& params)
      override;

  void DidGetRedirectForResourceRequest(
      const content::ResourceRedirectDetails& details) override;

  /**
   * Called as a callback from FraudProtectionService when rating of the URL
   * we asked to check becomes known.
   */
  void UrlChecked(const FraudUrlRating& rating);

  // Called by the |delegate_| when a navigation has been blocked due to e.g.
  // showing an http auth dialog.
  void OnNavigationBlocked();

  void AddObserver(Observer* observer) { observers_.AddObserver(observer); }
  void RemoveObserver(Observer* observer) {
    observers_.RemoveObserver(observer);
  }

 private:
  /**
   * Static function for creating a delegate. Each platform must have their own
   * implementation of this function.
   */
  static FraudProtectionDelegate* CreateDelegate(FraudProtection* owner);

  friend class content::WebContentsUserData<FraudProtection>;

  explicit FraudProtection(content::WebContents* web_contents);

  void CheckUrl(const GURL& url);

  void ShowFraudWarningPage(const FraudUrlRating& rating);

  void OnFraudWarningShown(content::WebContents* web_contents) override;
  void OnFraudWarningDismissed(content::WebContents* web_contents) override;

  content::WebContents* web_contents_;
  std::unique_ptr<FraudUrlRating> deferred_warning_page_;
  std::unique_ptr<FraudProtectionDelegate> delegate_;
  bool navigation_entry_commited_;
  bool provisional_load_failed;
  enum NavigationBlockingState {
    NO_BLOCKING,
    NEEDS_UNBLOCKING_TO_FINISH,
    FORCED_UNBLOCKING
  };
  NavigationBlockingState navigation_blocking_state_;

  // Observers
  base::ObserverList<FraudProtection::Observer> observers_;

  base::WeakPtrFactory<FraudProtection> weakptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FraudProtection);
};

}  // namespace opera

#endif  // COMMON_FRAUD_PROTECTION_UI_FRAUD_PROTECTION_H_
