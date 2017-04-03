// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/browser/articles/article_overlay.h"

#include <string>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "content/public/browser/android/content_view_core.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "jni/ArticleOverlay_jni.h"
#include "url/gurl.h"

#include "chill/common/opera_switches.h"
#include "chill/browser/articles/article.h"
#include "common/settings/settings_manager.h"

namespace {

const char kURLPathSeparator[] = "/";
const char kTranscoderPath[] = "news/detail";

GURL GetTranscoderServiceBaseUrl(const std::string& default_host) {
  GURL transcoder_service_base_url;
#ifndef PUBLIC_BUILD
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  GURL custom_transcoding_server(command_line->GetSwitchValueASCII(
      switches::kCustomArticleTranscodingServer));
  transcoder_service_base_url.Swap(&custom_transcoding_server);
#endif
  if (transcoder_service_base_url.is_empty())
    transcoder_service_base_url = GURL(default_host);

  std::string path;
  base::TrimString(transcoder_service_base_url.path(), kURLPathSeparator,
                   &path);
  path += kURLPathSeparator;
  path += kTranscoderPath;

  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  return transcoder_service_base_url.ReplaceComponents(replacements);
}

}  // namespace

namespace opera {

struct ArticleOverlay::ArticleState {
  Article article;
  bool transcoded_mode;

  ArticleState(JNIEnv* env, const base::android::JavaParamRef<jobject>& article)
      : article(env, article), transcoded_mode(false) {}
};

void ArticleOverlay::ArticleStateList::Insert(
    int index, std::unique_ptr<ArticleState> article) {
  articles_[index + offset_] = std::move(article);
}

ArticleOverlay::ArticleState* ArticleOverlay::ArticleStateList::Find(
    int index) {
  auto article = articles_.find(index + offset_);
  if (article == articles_.end()) return nullptr;
  return article->second.get();
}

void ArticleOverlay::ArticleStateList::PruneFront(int to_exclusive) {
  offset_ += to_exclusive;
  articles_.erase(articles_.begin(), articles_.lower_bound(offset_));
}

void ArticleOverlay::ArticleStateList::PruneBack(int from_exclusive) {
  articles_.erase(
      articles_.upper_bound(from_exclusive + offset_), articles_.end());
}

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ArticleOverlay);

ArticleOverlay::ArticleOverlay(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref,
    const GURL& transcoder_service_base_url,
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      java_ref_(env, java_ref),
      transcoder_service_base_url_(transcoder_service_base_url) {
  web_contents->SetUserData(UserDataKey(), this);
  DCHECK(transcoder_service_base_url_.is_valid());

  const content::NavigationController* controller =
      &web_contents->GetController();
  registrar_.Add(this, content::NOTIFICATION_NAV_LIST_PRUNED,
                 content::Source<content::NavigationController>(controller));
}

void ArticleOverlay::Observe(int type,
                             const content::NotificationSource& source,
                             const content::NotificationDetails& details) {
  if (type == content::NOTIFICATION_NAV_LIST_PRUNED) {
    // Erase all articles having indices larger than the current index
    // except for the pending article when the history is being pruned
    // from the back which happens when we go back in history and browse
    // something new.
    // When the history entry number exceeds a limit
    // (kMaxSessionHistoryEntries), the history is pruned from the front.
    // We need to adjust all indices and prune off the negative ones.
    // @see NotifyPrunedEntries in navigation_controller_impl
    const content::PrunedDetails* pd =
        content::Details<content::PrunedDetails>(details).ptr();
    content::NavigationController& controller = web_contents()->GetController();
    int index = controller.GetCurrentEntryIndex();

    if (pending_article_index_.has_value()) {
      if (pd->from_front) {
        pending_article_index_ = pending_article_index_.value() - pd->count;
        DCHECK(pending_article_index_ == index + 1);
      } else {
        DCHECK(pending_article_index_ == index + 1);
        // Exclude the pending article
        index++;
      }
    }

    if (pd->from_front) {
      articles_.PruneFront(pd->count);
    } else {
      articles_.PruneBack(index);
    }
  }
}

void ArticleOverlay::WebContentsDestroyed() {
  const content::NavigationController* controller =
      &web_contents()->GetController();
  registrar_.Remove(this, content::NOTIFICATION_NAV_LIST_PRUNED,
                    content::Source<content::NavigationController>(controller));
}

// static
base::android::ScopedJavaLocalRef<jobject> ArticleOverlay::FromWebContents(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_web_contents) {
  ArticleOverlay* overlay =
      WebContentsUserData<ArticleOverlay>::FromWebContents(
          content::WebContents::FromJavaWebContents(java_web_contents));

  if (overlay) {
    return base::android::ScopedJavaLocalRef<jobject>(overlay->java_ref_);
  }

  return base::android::ScopedJavaLocalRef<jobject>();
}

base::android::ScopedJavaLocalRef<jobject> FromWebContents(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& java_class,
    const base::android::JavaParamRef<jobject>& java_web_contents) {
  return ArticleOverlay::FromWebContents(env, java_web_contents);
}

base::android::ScopedJavaLocalRef<jobject> ArticleOverlay::GetVisibleArticle(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref) {
  ArticleState* article = GetVisibleArticle();
  if (article) {
    return base::android::ScopedJavaLocalRef<jobject>(
        article->article.GetObject());
  } else {
    return base::android::ScopedJavaLocalRef<jobject>();
  }
}

ArticleOverlay::ArticleState* ArticleOverlay::GetVisibleArticle() {
  content::NavigationController& controller = web_contents()->GetController();
  return articles_.Find(controller.GetCurrentEntryIndex());
}

void ArticleOverlay::OpenArticle(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref,
    const base::android::JavaParamRef<jobject>& article,
    jboolean transcoded_mode) {
  std::unique_ptr<ArticleState> pending_article(new ArticleState(env, article));

  pending_article->transcoded_mode = transcoded_mode;

  GURL url = pending_article->transcoded_mode
                 ? BuildTranscodeURL(pending_article->article.GetNewsId(env))
                 : GURL(pending_article->article.GetOriginalUrl(env));

  Java_ArticleOverlay_attachContentViewCore(
      env, java_ref.obj(),
      content::ContentViewCore::FromWebContents(web_contents())
          ->GetJavaObject()
          .obj(),
      pending_article->transcoded_mode);

  content::NavigationController& controller = web_contents()->GetController();
  controller.LoadURL(url, content::Referrer(), ui::PAGE_TRANSITION_LINK,
                     std::string());

  pending_article_index_ = controller.GetCurrentEntryIndex() + 1;
  articles_.Insert(pending_article_index_.value(), std::move(pending_article));

  UpdateReadingModeButtonState();
}

void ArticleOverlay::ToggleReadingMode(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref) {
  ArticleState* visible_article = GetVisibleArticle();
  DCHECK(visible_article);
  if (!visible_article)
    return;

  content::NavigationController::LoadURLParams params(
      GURL(visible_article->article.GetOriginalUrl(env)));
  if (!visible_article->transcoded_mode) {
    params.url = BuildTranscodeURL(visible_article->article.GetNewsId(env));
  }
  visible_article->transcoded_mode = !visible_article->transcoded_mode;

  params.should_replace_current_entry = true;
  // Sets both Pending and Visible entry in controller to new entry
  content::NavigationController& controller = web_contents()->GetController();
  controller.LoadURLWithParams(params);

  UpdateReadingModeButtonState();
  NotifyOnReadingModeChanged();
}

void ArticleOverlay::ActivateReadingMode(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref) {
  ArticleState* visible_article = GetVisibleArticle();
  if (visible_article && !visible_article->transcoded_mode) {
    ToggleReadingMode(env, java_ref);
  }
}

void ArticleOverlay::ReloadArticle(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref) {
  NotifyOnReadingModeChanged();
  content::NavigationController& controller = web_contents()->GetController();
  controller.ReloadBypassingCache(false);
}

jboolean ArticleOverlay::IsShowingArticleForVisibleEntry(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref) {
  return !!GetVisibleArticle();
}

void ArticleOverlay::WasShown() {
  UpdateReadingModeButtonState();
  NotifySetActive();
}

void ArticleOverlay::WasHidden() {
  Java_ArticleOverlay_updateReadingModeState(
      base::android::AttachCurrentThread(), java_ref_.obj(), false, false);
  NotifySetActive();
}

// Detect page load and notify Java side abouth whether we're loading an
// article or not
void ArticleOverlay::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  if (!load_details.is_main_frame)
    return;

  DCHECK(!pending_article_index_.has_value() || pending_article_index_
      == web_contents()->GetController().GetCurrentEntryIndex());
  pending_article_index_.reset();
  NotifySetActive();
  UpdateReadingModeButtonState();
}

void ArticleOverlay::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!GetVisibleArticle())
    return;

  if (!navigation_handle->IsInMainFrame())
    return;
  if (navigation_handle->IsErrorPage()) {
    std::string error_description =
        net::ErrorToShortString(navigation_handle->GetNetErrorCode());
    NotifyLoadingFailed(error_description);
  }
}

void ArticleOverlay::DidFirstVisuallyNonEmptyPaint() {
  if (GetVisibleArticle()) {
    Java_ArticleOverlay_onLoaded(base::android::AttachCurrentThread(),
                                 java_ref_.obj());
  }
}

bool ArticleOverlay::Register(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void ArticleOverlay::UpdateReadingModeButtonState() {
  ArticleState* visible_article = GetVisibleArticle();
  if (visible_article) {
    Java_ArticleOverlay_updateReadingModeState(
        base::android::AttachCurrentThread(), java_ref_.obj(), true,
        visible_article->transcoded_mode);
  } else {
    Java_ArticleOverlay_updateReadingModeState(
        base::android::AttachCurrentThread(), java_ref_.obj(), false, false);
  }
}

void ArticleOverlay::NotifyOnReadingModeChanged() {
  ArticleState* visible_article = GetVisibleArticle();
  if (visible_article) {
    Java_ArticleOverlay_onReadingModeChanged(
        base::android::AttachCurrentThread(), java_ref_.obj(),
        visible_article->transcoded_mode);
  }
}

void ArticleOverlay::NotifySetActive() {
  bool is_showing = false;
  content::RenderFrameHost* rwh = web_contents()->GetMainFrame();
  if (rwh) {
    content::RenderWidgetHostView* rwhv = rwh->GetView();
    is_showing = rwhv && rwhv->IsShowing();
  }

  Java_ArticleOverlay_setActive(base::android::AttachCurrentThread(),
                                java_ref_.obj(),
                                GetVisibleArticle() && is_showing);
}

void ArticleOverlay::NotifyLoadingFailed(const std::string& error_description) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> java_error_description(
      base::android::ConvertUTF8ToJavaString(env, error_description));
  Java_ArticleOverlay_loadingFailed(env, java_ref_.obj(),
                                    java_error_description.obj());
}

GURL ArticleOverlay::BuildTranscodeURL(std::string news_id) {
  std::string path;
  base::TrimString(transcoder_service_base_url_.path(), kURLPathSeparator,
                   &path);
  path += kURLPathSeparator;
  path += news_id;
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  return transcoder_service_base_url_.ReplaceComponents(replacements);
}

jlong Init(JNIEnv* env,
           const base::android::JavaParamRef<jobject>& java_ref,
           const base::android::JavaParamRef<jobject>& j_web_contents,
           const base::android::JavaParamRef<jstring>& j_transcoder_host) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(j_web_contents);

  return reinterpret_cast<intptr_t>(new ArticleOverlay(
      env, java_ref,
      GetTranscoderServiceBaseUrl(
          base::android::ConvertJavaStringToUTF8(j_transcoder_host)),
      web_contents));
}

}  // namespace opera
