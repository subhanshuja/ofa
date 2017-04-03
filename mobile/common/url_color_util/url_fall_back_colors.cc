// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/url_color_util/url_fall_back_colors.h"

namespace mobile {
namespace {

uint32_t kColorTable[] = {
    0xFF2D85A4, 0xFF963A97, 0xFF3A4AB4, 0xFF863AB4, 0xFFC846C9, 0xFFF456A4,
    0xFF26E29C, 0xFF3CA4DF, 0xFF3A83E3, 0xFF4056E3, 0xFF9058F0, 0xFF5E95D5,
    0xFF41BBF5, 0xFF5E5BE7, 0xFF1B7EFF, 0xFF8A45FF, 0xFFB42424, 0xFFFF881C,
    0xFF1C941B, 0xFF189365, 0xFF189196, 0xFF982F2F, 0xFFD30000, 0xFFEE3131,
    0xFFF0C92C, 0xFF3AB43A, 0xFF3AB487, 0xFFF45C00, 0xFF8BD34B, 0xFF4AD449,
    0xFFF2B722, 0xFF2DBBB1 };

bool in_table_range(unsigned int nr) {
  return nr < (sizeof(kColorTable) / sizeof(kColorTable[0]));
}

}  // namespace

/* static */
uint32_t URLFallBackColors::GetFallBackColorNumber(unsigned int nr) {
  if (!in_table_range(nr))
    return 0;

  return kColorTable[nr];
}

}  // namespace mobile
