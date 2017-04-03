// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/sessions/parent_window.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "common/ini_parser/ini_parser.h"

namespace opera {
namespace common {
namespace migration {

const int kInvalidValue = -1;

ParentWindow::ParentWindow()
  : x_(kInvalidValue),
    y_(kInvalidValue),
    width_(kInvalidValue),
    height_(kInvalidValue),
    state_(STATE_ERROR) {
}

ParentWindow::ParentWindow(const ParentWindow&) = default;

ParentWindow::~ParentWindow() {
}

bool ParentWindow::SetFromIni(const IniParser &parser, int section_number) {
  std::string section_name = base::IntToString(section_number);
  x_ = parser.GetAs<int>(section_name, "x", kInvalidValue);
  y_ = parser.GetAs<int>(section_name, "y", kInvalidValue);
  width_ = parser.GetAs<int>(section_name, "w", kInvalidValue);
  height_ = parser.GetAs<int>(section_name, "h", kInvalidValue);
  state_ = static_cast<State>(parser.GetAs<int>(section_name, "state", 0));

  return x_ != kInvalidValue &&
         y_ != kInvalidValue &&
         width_ != kInvalidValue &&
         height_ != kInvalidValue &&
         state_ < STATE_ERROR;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
