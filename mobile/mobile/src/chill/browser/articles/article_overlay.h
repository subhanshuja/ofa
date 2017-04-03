// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_BROWSER_ARTICLES_ARTICLE_OVERLAY_H_
#define CHILL_BROWSER_ARTICLES_ARTICLE_OVERLAY_H_

#include <jni.h>

#include <string>
#include <map>
#include <memory>

#include "base/android/scoped_java_ref.h"
#include "base/optional.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
class RenderFrameHost;
class WebContents;
}

namespace opera {

// Manipulates a WebContents and its ContentViewCore view container to provide
// an augmented user experience when viewing news reader articles.
//
// While the ArticleOverlay is attached to a WebContents, any navigation entry
// corresponding to a loaded article can be replaced with a different navigation
// entry loading the same article, but in a different "mode".
//
// The WebContents container view attached to the Android view hierarchy is
// given a child overlay through ArticleOverlayViewModel which can draw over the
// web contents surface.
class ArticleOverlay : public content::WebContentsUserData<ArticleOverlay>,
                       public content::WebContentsObserver,
                       public content::NotificationObserver {
 public:
  ArticleOverlay(JNIEnv* env,
                 const base::android::JavaParamRef<jobject>& obj,
                 const GURL& transcoder_service_base_url,
                 content::WebContents* web_contents);

  static base::android::ScopedJavaLocalRef<jobject> FromWebContents(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& java_web_contents);

  void OpenArticle(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& java_ref,
                   const base::android::JavaParamRef<jobject>& article,
                   jboolean transcoded_mode);

  void ToggleReadingMode(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& java_ref);

  void ActivateReadingMode(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& java_ref);

  void ReloadArticle(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& java_ref);

  jboolean IsShowingArticleForVisibleEntry(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& java_ref);

  base::android::ScopedJavaLocalRef<jobject> GetVisibleArticle(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& java_ref);

  // WebContentsObserver methods

  void WasShown() override;
  void WasHidden() override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFirstVisuallyNonEmptyPaint() override;
  void WebContentsDestroyed() override;

  // NotificationObserver methods

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  static bool Register(JNIEnv* env);

 private:
  struct ArticleState;

  // Wraps around std::map<int, std::unique_ptr<ArticleState>> and
  // provides methods for pruning articles when history is pruned.
  class ArticleStateList {
   public:
    void Insert(int index, std::unique_ptr<ArticleState> article);
    ArticleState* Find(int index);
    void PruneFront(int to_exclusive);
    void PruneBack(int from_exclusive);

   private:
    // Map of NavigationEntry index to articles
    std::map<int, std::unique_ptr<ArticleState>> articles_;
    int offset_;
  };

  ArticleState* GetVisibleArticle();

  void UpdateReadingModeButtonState();
  void NotifyOnReadingModeChanged();
  void NotifySetActive();
  void NotifyLoadingFailed(const std::string& error_description);

  GURL BuildTranscodeURL(std::string news_id);

  friend class content::WebContentsUserData<ArticleOverlay>;

  ArticleStateList articles_;

  base::android::ScopedJavaGlobalRef<jobject> java_ref_;

  GURL transcoder_service_base_url_;

  content::NotificationRegistrar registrar_;

  base::Optional<int> pending_article_index_;

  DISALLOW_COPY_AND_ASSIGN(ArticleOverlay);
};

}  // namespace opera

#endif  // CHILL_BROWSER_ARTICLES_ARTICLE_OVERLAY_H_
