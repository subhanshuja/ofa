// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_FAVORITES_THUMBNAIL_REQUEST_INTERCEPTOR_H_
#define CHILL_BROWSER_FAVORITES_THUMBNAIL_REQUEST_INTERCEPTOR_H_

#include <jni.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "third_party/skia/include/core/SkBitmap.h"

#include "mobile/common/favorites/favorites_delegate.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace opera {

/**
 * Keeps bitmap icons associated with the given URLs to be used for speed-dial
 * items. Intercepts thumbnail requests sent to Bream by listening to
 * FavoritesDelegate interface. When a thumbnail request is received, the bitmap
 * icon associated with the given URL is saved in PNG format under the given
 * file name. Forwards all other requests to Bream.
 */
class ThumbnailRequestInterceptor {
 public:
  static ThumbnailRequestInterceptor* GetInstance();

  void Initialize(mobile::FavoritesDelegate* delegate);

  // Receive an incoming thumbnail request. Find the associated thumbnail bitmap
  // and start processing the request in a background thread where the bitmap
  // is converted to PNG and saved under the given name. Inform the receiver of
  // the result of the processing when done. If no bitmap found, defer request
  // until a bitmap associated with the given url is received.
  void RequestThumbnail(int64_t id,
                        const std::string& url,
                        const std::string& file_name,
                        bool fetch_title,
                        mobile::FavoritesDelegate::ThumbnailReceiver* receiver);

  // Receive an incoming cancel request for a previously received thumbnail
  // request. Causes the PNG file to be deleted if it was already saved.
  void CancelThumbnailRequest(int64_t id);

  // Receive thumbnail info associated with a given url. Stores a bitmap and a
  // title for a given url which are kept until consumed by a RequestThumbnail
  // call with a matching url.
  void SetThumbnailForUrl(jobject bitmap,
                          const std::string& title,
                          const std::string& url);

  // Returns true if there is a deferred request waiting for a thumbnail for a
  // given url.
  bool HasDeferredRequest(const std::string& url);

 private:
  friend struct base::DefaultSingletonTraits<ThumbnailRequestInterceptor>;

  ThumbnailRequestInterceptor();
  ~ThumbnailRequestInterceptor();

  struct Thumbnail {
    std::unique_ptr<SkBitmap> bitmap;
    std::string title;

    Thumbnail() {}

    Thumbnail(std::unique_ptr<SkBitmap> bitmap, std::string title)
        : bitmap(std::move(bitmap)), title(title) {}
  };

  struct Request {
    int64_t id;
    std::string url;
    std::string file_name;
    bool fetch_title;
    mobile::FavoritesDelegate::ThumbnailReceiver* receiver;

    Request() : id(-1), fetch_title(false), receiver(nullptr) {}

    Request(int64_t id,
            std::string url,
            std::string file_name,
            bool fetch_title,
            mobile::FavoritesDelegate::ThumbnailReceiver* receiver)
        : id(id),
          url(url),
          file_name(file_name),
          fetch_title(fetch_title),
          receiver(receiver) {}
  };

  void ProcessRequest(std::unique_ptr<SkBitmap> thumbnail_bitmap,
                      std::string file_path,
                      int64_t id,
                      std::string url,
                      std::string title,
                      mobile::FavoritesDelegate::ThumbnailReceiver* receiver);

  void Finished(bool succeeded,
                std::string file_path,
                int64_t id,
                std::string url,
                std::string title,
                mobile::FavoritesDelegate::ThumbnailReceiver* receiver);

  void OnDeferredRequestAdded(const std::string& url);
  void OnDeferredRequestRemoved(const std::string& url);

  // Path to the thumbnails directory.
  std::string thumbnails_directory_;

  // Our delegate intercepting thumbnail requests sent to Bream.
  std::unique_ptr<mobile::FavoritesDelegate> delegate_;

  // Map of thumbnail bitmaps keyed by URL string. Bitmaps are added to this map
  // once a favicon associated with the URL is downloaded or generated.
  std::unordered_map<std::string, Thumbnail> thumbnails_;

  // Ids of requests that are currently in progress.
  std::unordered_set<int64_t> active_requests_;

  // Request that are waiting for thumbnail bitmaps.
  std::vector<Request> deferred_requests_;

  // Number of deferred requests made for each url. Used for fast query.
  std::unordered_map<std::string, int> deferred_requests_by_url_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_FAVORITES_THUMBNAIL_REQUEST_INTERCEPTOR_H_
