// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/webapps/favicon_selector.h"

#include <limits>

#include "base/bind.h"
#include "content/public/browser/web_contents.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace opera {

FaviconSelector::FaviconSelector(content::WebContents* web_contents)
    : web_contents_(web_contents),
      favicon_selector_in_progress_(false),
      ideal_icon_size_in_px_(0),
      weak_ptr_factory_(this) {}

void FaviconSelector::FindBestMatchingIconDP(
    const std::vector<WebApplicationInfo::IconInfo>& icons,
    int ideal_icon_size_in_dp,
    const FaviconSelectedCallback& callback) {
  const float device_scale_factor =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  int ideal_icon_size_in_px =
      static_cast<int>(round(ideal_icon_size_in_dp * device_scale_factor));
  FindBestMatchingIconPX(icons, ideal_icon_size_in_px, callback);
}

void FaviconSelector::FindBestMatchingIconPX(
    const std::vector<WebApplicationInfo::IconInfo>& icons,
    int ideal_icon_size_in_px,
    const FaviconSelectedCallback& callback) {
  DCHECK(!favicon_selector_in_progress_);

  ideal_icon_size_in_px_ = ideal_icon_size_in_px;
  DCHECK_GT(ideal_icon_size_in_px_, 0);

  favicon_selector_in_progress_ = true;
  callback_ = callback;
  candidates_.clear();
  best_candidate_ = WebApplicationInfo::IconInfo();
  best_candidate_.height = std::numeric_limits<int>::max();
  biggest_icon_ = WebApplicationInfo::IconInfo();

  // Build cadidates list.
  for (const WebApplicationInfo::IconInfo& icon : icons) {
    if (icon.url.is_valid()) {
      if (icon.height == ideal_icon_size_in_px_) {
        // Found exact match.
        best_candidate_ = icon;
        DoneProcessingCandidates();
        return;
      }
      candidates_.push_back(icon);
    }
  }

  ProcessCandidates();
}

void FaviconSelector::ProcessCandidates() {
  while (!candidates_.empty()) {
    WebApplicationInfo::IconInfo& candidate = candidates_.back();
    if (candidate.height <= 0) {
      // No size info available. Need to download first.
      web_contents_->DownloadImage(
          candidate.url, true, ideal_icon_size_in_px_, false,
          base::Bind(&FaviconSelector::OnFaviconDownloaded,
                     weak_ptr_factory_.GetWeakPtr()));
      return;
    }

    if (candidate.height == ideal_icon_size_in_px_) {
      // Found exact match.
      best_candidate_ = candidate;
      break;
    } else if (candidate.height > ideal_icon_size_in_px_ &&
               candidate.height < best_candidate_.height) {
      // Found a better candidate.
      best_candidate_ = candidate;
    }

    if (!best_candidate_.url.is_valid() &&
        candidate.height > biggest_icon_.height) {
      // No candidate yet. Keep the biggest icon we found so far.
      biggest_icon_ = candidate;
    }

    // Consume candidate.
    candidates_.pop_back();
  }

  DoneProcessingCandidates();
}

void FaviconSelector::DoneProcessingCandidates() {
  // Use the best candidate if available. Use the biggest icon found otherwise.
  const WebApplicationInfo::IconInfo& selected_icon =
      best_candidate_.url.is_valid() ? best_candidate_ : biggest_icon_;

  if (!selected_icon.url.is_valid()) {
    // No icon found.
    NotifyClient(SkBitmap());
    return;
  }

  if (!selected_icon.data.drawsNothing()) {
    // No need to download. Return existing bitmap.
    NotifyClient(selected_icon.data);
    return;
  }

  web_contents_->DownloadImage(
      selected_icon.url, true, ideal_icon_size_in_px_, false,
      base::Bind(&FaviconSelector::OnFinalFaviconDownloaded,
                 weak_ptr_factory_.GetWeakPtr()));
}

void FaviconSelector::OnFaviconDownloaded(int id,
                                          int http_status_code,
                                          const GURL& url,
                                          const std::vector<SkBitmap>& bitmaps,
                                          const std::vector<gfx::Size>& sizes) {
  DCHECK(!candidates_.empty());

  if (!bitmaps.empty()) {
    // There might be multiple bitmaps returned. The one to pick is bigger or
    // equal to the preferred size. |bitmaps| is ordered from bigger to smaller.
    int preferred_bitmap_index = 0;
    for (size_t i = 0; i < bitmaps.size(); ++i) {
      if (bitmaps[i].height() < ideal_icon_size_in_px_)
        break;
      preferred_bitmap_index = i;
    }

    if (bitmaps[preferred_bitmap_index].height() > 0) {
      // Update current candidate from downloaded image.
      WebApplicationInfo::IconInfo& candidate = candidates_.back();
      candidate.data = bitmaps[preferred_bitmap_index];
      candidate.width = bitmaps[preferred_bitmap_index].width();
      candidate.height = bitmaps[preferred_bitmap_index].height();
    } else {
      // Consume current candidate.
      candidates_.pop_back();
    }
  } else {
    // Consume current candidate.
    candidates_.pop_back();
  }

  ProcessCandidates();
}

void FaviconSelector::OnFinalFaviconDownloaded(
    int id,
    int http_status_code,
    const GURL& url,
    const std::vector<SkBitmap>& bitmaps,
    const std::vector<gfx::Size>& sizes) {
  SkBitmap icon_bitmap;
  if (!bitmaps.empty()) {
    // There might be multiple bitmaps returned. The one to pick is bigger or
    // equal to the preferred size. |bitmaps| is ordered from bigger to smaller.
    int preferred_bitmap_index = 0;
    for (size_t i = 0; i < bitmaps.size(); ++i) {
      if (bitmaps[i].height() < ideal_icon_size_in_px_)
        break;
      preferred_bitmap_index = i;
    }
    icon_bitmap = bitmaps[preferred_bitmap_index];
  }

  NotifyClient(icon_bitmap);
}

void FaviconSelector::NotifyClient(const SkBitmap& bitmap) {
  favicon_selector_in_progress_ = false;
  callback_.Run(bitmap);
}

}  // namespace opera
