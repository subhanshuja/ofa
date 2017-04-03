// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified by Opera Software ASA
// @copied-from chrome/browser/manifest/manifest_icon_selector.cc
// @final-synchronized

#include "chill/browser/webapps/manifest_icon_selector.h"

#include <limits>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "components/mime_util/mime_util.h"
#include "content/public/browser/web_contents.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

using content::Manifest;

ManifestIconSelector::ManifestIconSelector(int ideal_icon_size_in_px,
                                           int minimum_icon_size_in_px)
    : ideal_icon_size_in_px_(ideal_icon_size_in_px),
      minimum_icon_size_in_px_(minimum_icon_size_in_px) {
}

bool ManifestIconSelector::IconSizesContainsPreferredSize(
    const std::vector<gfx::Size>& sizes) {
  for (size_t i = 0; i < sizes.size(); ++i) {
    if (sizes[i].height() != sizes[i].width())
      continue;
    if (sizes[i].width() == ideal_icon_size_in_px_)
      return true;
  }

  return false;
}

bool ManifestIconSelector::IconSizesContainsBiggerThanMinimumSize(
    const std::vector<gfx::Size>& sizes) {
  for (size_t i = 0; i < sizes.size(); ++i) {
    if (sizes[i].height() != sizes[i].width())
      continue;
    if (sizes[i].width() >= minimum_icon_size_in_px_)
      return true;
  }
  return false;
}

int ManifestIconSelector::FindBestMatchingIconForSize(
    const std::vector<content::Manifest::Icon>& icons) {
  int best_index = -1;
  int best_delta = std::numeric_limits<int>::min();

  for (size_t i = 0; i < icons.size(); ++i) {
    const std::vector<gfx::Size>& sizes = icons[i].sizes;
    for (size_t j = 0; j < sizes.size(); ++j) {
      if (sizes[j].height() != sizes[j].width())
        continue;
      int delta = sizes[j].width() - ideal_icon_size_in_px_;
      if (delta == 0)
        return i;
      if (sizes[i].width() < minimum_icon_size_in_px_)
        continue;
      if (best_delta > 0 && delta < 0)
        continue;
      if ((best_delta > 0 && delta < best_delta) ||
          (best_delta < 0 && delta > best_delta)) {
        best_index = i;
        best_delta = delta;
      }
    }
  }

  return best_index;
}

int ManifestIconSelector::FindBestMatchingIcon(
    const std::vector<content::Manifest::Icon>& icons) {
  int best_index = -1;
  int best_delta = std::numeric_limits<int>::min();

  for (size_t i = 0; i < icons.size(); ++i) {
    const std::vector<gfx::Size>& sizes = icons[i].sizes;
    for (size_t j = 0; j < sizes.size(); ++j) {
      if (sizes[j].height() != sizes[j].width())
        continue;
      int delta = sizes[j].width() - ideal_icon_size_in_px_;
      if (delta == 0)
        return i;
      if (best_delta > 0 && delta < 0)
        continue;
      if ((best_delta > 0 && delta < best_delta) ||
          (best_delta < 0 && delta > best_delta)) {
        best_index = i;
        best_delta = delta;
      }
    }
  }

  return best_index;
}


// static
bool ManifestIconSelector::IconSizesContainsAny(
    const std::vector<gfx::Size>& sizes) {
  for (size_t i = 0; i < sizes.size(); ++i) {
    if (sizes[i].IsEmpty())
      return true;
  }
  return false;
}

// static
std::vector<Manifest::Icon> ManifestIconSelector::FilterIconsByType(
    const std::vector<content::Manifest::Icon>& icons) {
  std::vector<Manifest::Icon> result;

  for (size_t i = 0; i < icons.size(); ++i) {
    if (icons[i].type.empty() ||
        mime_util::IsSupportedImageMimeType(base::UTF16ToUTF8(icons[i].type))) {
      result.push_back(icons[i]);
    }
  }

  return result;
}

// static
GURL ManifestIconSelector::FindBestMatchingIcon(
    const std::vector<Manifest::Icon>& unfiltered_icons,
    const int ideal_icon_size_in_dp,
    const int minimum_icon_size_in_dp) {
  DCHECK(minimum_icon_size_in_dp <= ideal_icon_size_in_dp);

  const float device_scale_factor =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  const int ideal_icon_size_in_px =
      static_cast<int>(round(ideal_icon_size_in_dp * device_scale_factor));
  const int minimum_icon_size_in_px =
      static_cast<int>(round(minimum_icon_size_in_dp * device_scale_factor));

  std::vector<Manifest::Icon> icons =
      ManifestIconSelector::FilterIconsByType(unfiltered_icons);

  ManifestIconSelector selector(ideal_icon_size_in_px,
                                minimum_icon_size_in_px);
  int index = selector.FindBestMatchingIcon(icons);
  if (index == -1)
    return GURL();
  return icons[index].src;
}
