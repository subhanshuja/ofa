// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/favorites/thumbnail_request_interceptor.h"

#include <cstdio>

#include "base/logging.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/codec/png_codec.h"

#include "mobile/common/favorites/favorites_delegate.h"

#include "chill/browser/favorites/thumbnail_request_interceptor_helper.h"

using mobile::FavoritesDelegate;

namespace {

/**
 * Intercepts thumbnail requests. Forwards all other requests to Bream.
 */
class FavoritesDelegateImpl : public FavoritesDelegate {
 public:
  explicit FavoritesDelegateImpl(FavoritesDelegate* forward_delegate)
      : forward_delegate_(forward_delegate) {}
  ~FavoritesDelegateImpl() override {}

  void SetRequestGraphicsSizes(int icon_size, int thumb_size) override {
    forward_delegate_->SetRequestGraphicsSizes(icon_size, thumb_size);
  }

  // This is where we intercept the thumbnail requests coming from the common
  // favorites system. A request is sent for each speed-dial item with missing
  // thumbnail (associated png file). Requests are sent again if Opera restarts
  // without updating the thumbnails. We rely on this assumption to build a
  // deferred requests list for missing thumbnails even after Opera restarts.
  void RequestThumbnail(int64_t id,
                        const char* url,
                        const char* path,
                        bool fetch_title,
                        ThumbnailReceiver* receiver) override {
    opera::ThumbnailRequestInterceptor::GetInstance()->RequestThumbnail(
        id, url, path, fetch_title, receiver);
  }

  void CancelThumbnailRequest(int64_t id) override {
    opera::ThumbnailRequestInterceptor::GetInstance()->CancelThumbnailRequest(
        id);
  }

  void SetPartnerContentInterface(PartnerContentInterface* interface) override {
    forward_delegate_->SetPartnerContentInterface(interface);
  }

  void OnPartnerContentActivated() override {
    forward_delegate_->OnPartnerContentActivated();
  }

  void OnPartnerContentRemoved(int32_t partner_id) override {
    forward_delegate_->OnPartnerContentRemoved(partner_id);
  }

  void OnPartnerContentEdited(int32_t partner_id) override {
    forward_delegate_->OnPartnerContentEdited(partner_id);
  }

  const char* GetPathSuffix() const override {
    return forward_delegate_->GetPathSuffix();
  }

 private:
  FavoritesDelegate* forward_delegate_;
};

}  // namespace

void ThumbnailRequestInterceptor_SetFavoritesDelegate(
    FavoritesDelegate* delegate) {
  opera::ThumbnailRequestInterceptor::GetInstance()->Initialize(delegate);
}

namespace opera {

ThumbnailRequestInterceptor::ThumbnailRequestInterceptor()
    : delegate_(nullptr) {}

ThumbnailRequestInterceptor::~ThumbnailRequestInterceptor() {
  SetFavoritesDelegate(nullptr);
}

ThumbnailRequestInterceptor* ThumbnailRequestInterceptor::GetInstance() {
  return base::Singleton<ThumbnailRequestInterceptor>::get();
}

void ThumbnailRequestInterceptor::Initialize(FavoritesDelegate* delegate) {
  base::FilePath app_base_path;
  PathService::Get(base::DIR_ANDROID_APP_DATA, &app_base_path);
  thumbnails_directory_ = app_base_path.DirName().Append("files/").value();

  delegate_.reset(new FavoritesDelegateImpl(delegate));
  SetFavoritesDelegate(delegate_.get());
}

void ThumbnailRequestInterceptor::RequestThumbnail(
    int64_t id,
    const std::string& url,
    const std::string& file_name,
    bool fetch_title,
    FavoritesDelegate::ThumbnailReceiver* receiver) {
  auto it = thumbnails_.find(url);
  if (it == thumbnails_.end()) {
    // No thumbnail found. Defer the request until user visits the url.
    deferred_requests_.push_back(
        Request(id, url, file_name, fetch_title, receiver));
    OnDeferredRequestAdded(url);
    return;
  }

  std::unique_ptr<SkBitmap> thumbnail_bitmap = std::move(it->second.bitmap);
  std::string title = fetch_title ? it->second.title : "";
  thumbnails_.erase(it);

  std::string file_path = thumbnails_directory_ + file_name;

  active_requests_.insert(id);

  content::BrowserThread::PostTask(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&ThumbnailRequestInterceptor::ProcessRequest,
                 base::Unretained(this), base::Passed(&thumbnail_bitmap),
                 file_path, id, url, title, receiver));
}

void ThumbnailRequestInterceptor::CancelThumbnailRequest(int64_t id) {
  auto active_it = active_requests_.find(id);
  if (active_it != active_requests_.end())
    active_requests_.erase(active_it);

  for (auto deferred_it = deferred_requests_.begin();
       deferred_it != deferred_requests_.end(); ++deferred_it) {
    if (deferred_it->id == id) {
      OnDeferredRequestRemoved(deferred_it->url);
      deferred_requests_.erase(deferred_it);
      break;
    }
  }
}

void ThumbnailRequestInterceptor::SetThumbnailForUrl(jobject bitmap,
                                                     const std::string& title,
                                                     const std::string& url) {
  SkBitmap skbmp = gfx::CreateSkBitmapFromJavaBitmap(gfx::JavaBitmap(bitmap));
  if (skbmp.drawsNothing())
    return;

  if (HasDeferredRequest(url)) {
    // Process all deferred request awaiting for the given URL.
    for (auto it = deferred_requests_.begin();
         it != deferred_requests_.end();) {
      if (it->url == url) {
        std::unique_ptr<SkBitmap> thumbnail_bitmap;
        thumbnail_bitmap.reset(new SkBitmap(skbmp));
        thumbnails_[url] = Thumbnail(std::move(thumbnail_bitmap), title);

        RequestThumbnail(it->id, it->url, it->file_name, it->fetch_title,
                         it->receiver);
        OnDeferredRequestRemoved(it->url);
        it = deferred_requests_.erase(it);
      } else {
        ++it;
      }
    }
  } else {
    // No deferred request was found, keep the thumbnail for a future request.
    std::unique_ptr<SkBitmap> thumbnail_bitmap;
    thumbnail_bitmap.reset(new SkBitmap(skbmp));
    thumbnails_[url] = Thumbnail(std::move(thumbnail_bitmap), title);
  }
}

bool ThumbnailRequestInterceptor::HasDeferredRequest(const std::string& url) {
  auto it = deferred_requests_by_url_.find(url);
  return it != deferred_requests_by_url_.end();
}

void ThumbnailRequestInterceptor::ProcessRequest(
    std::unique_ptr<SkBitmap> thumbnail_bitmap,
    std::string file_path,
    int64_t id,
    std::string url,
    std::string title,
    FavoritesDelegate::ThumbnailReceiver* receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);

  bool succeeded = false;
  do {
    std::vector<unsigned char> png_data;
    if (!gfx::PNGCodec::EncodeBGRASkBitmap(*thumbnail_bitmap.get(), false,
                                           &png_data)) {
      LOG(ERROR) << "Failed to encode png for thumbnail file " << file_path;
      break;
    }

    base::ScopedFILE file;
    file.reset(fopen(file_path.c_str(), "w"));
    if (!file) {
      LOG(ERROR) << "Failed to create thumbnail file \"" << file_path
                 << "\" errno=" << errno;
      break;
    }

    const void* data = reinterpret_cast<const void*>(&png_data[0]);
    if (fwrite(data, png_data.size(), 1, file.get()) != 1) {
      LOG(ERROR) << "Failed to write png data to thumbnail file " << file_path;
      break;
    }

    succeeded = true;
  } while (0);

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&ThumbnailRequestInterceptor::Finished, base::Unretained(this),
                 succeeded, file_path, id, url, title, receiver));
}

void ThumbnailRequestInterceptor::Finished(
    bool succeeded,
    std::string file_path,
    int64_t id,
    std::string url,
    std::string title,
    FavoritesDelegate::ThumbnailReceiver* receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // React if the request was canceled.
  auto it = active_requests_.find(id);
  if (it == active_requests_.end()) {
    const base::FilePath path(file_path);
    base::DeleteFile(path, false);
    return;
  }
  active_requests_.erase(it);

  if (succeeded) {
    receiver->Finished(id, title.c_str());
  } else {
    // Report all errors as temporary to keep old behavior
    receiver->Error(id, true);
  }
}

void ThumbnailRequestInterceptor::OnDeferredRequestAdded(
    const std::string& url) {
  deferred_requests_by_url_[url]++;
}

void ThumbnailRequestInterceptor::OnDeferredRequestRemoved(
    const std::string& url) {
  auto it = deferred_requests_by_url_.find(url);
  DCHECK(it != deferred_requests_by_url_.end());
  if (it->second > 1) {
    it->second -= 1;
  } else {
    deferred_requests_by_url_.erase(it);
  }
}

}  // namespace opera
