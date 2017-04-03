// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/sessions/tab_session.h"

#include <algorithm>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_constants.h"

#include "common/ini_parser/ini_parser.h"

namespace opera {
namespace common {
namespace migration {

TabSession::TabSession()
  : type_(UNKNOWN),
    current_history_(0),
    active_(false),
    tab_index_(-1),
    stack_index_(-1),
    x_(-1),
    y_(-1),
    width_(-1),
    height_(-1),
    show_scrollbars_(false),
    scale_(-1),
    group_id_(-1),
    group_active_(false),
    hidden_(false) {
}

TabSession::TabSession(const TabSession&) = default;

TabSession::~TabSession() {
}

bool TabSession::SetFromIni(const IniParser &parser, int section_number) {
  std::string section_name = base::IntToString(section_number);
  int type = parser.GetAs<int>(section_name, "type", -1);
  if (type < 0 || type > static_cast<int>(UNKNOWN))
    type_ = UNKNOWN;
  else
    type_ = static_cast<Type>(type);
  if (type_ == PANEL_WINDOW)
    panel_type_ = parser.Get(section_name, "panel type", "");
  active_ = parser.GetAs<bool>(section_name, "active", false);
  tab_index_ = parser.GetAs<int>(section_name, "position", -1);
  stack_index_ = parser.GetAs<int>(section_name, "stack position", -1);
  x_ = parser.GetAs<int>(section_name, "x", -1);
  y_ = parser.GetAs<int>(section_name, "y", -1);
  width_ = parser.GetAs<int>(section_name, "w", -1);
  height_ = parser.GetAs<int>(section_name, "h", -1);
  show_scrollbars_ = parser.GetAs<bool>(section_name, "show scrollbars", false);
  scale_ = parser.GetAs<int>(section_name, "scale", -1);
  encoding_ = parser.Get(section_name, "encoding", "");
  group_id_ = parser.GetAs<int>(section_name, "group", -1);
  group_active_ = parser.GetAs<bool>(section_name, "group-active", false);
  hidden_ = parser.GetAs<bool>(section_name, "hidden", false);

  bool history_ok =
      type_ == DOCUMENT_WINDOW ?
        ParseHistory(parser, section_number) :
        true;  // This is not a document tab, it doesn't have history, it's fine
  return tab_index_ != -1
      && stack_index_ != -1
      && x_ != -1
      && y_ != -1
      && width_ != -1
      && height_ != -1
      && type_ != UNKNOWN
      && history_ok;
}

bool TabSession::ParseHistory(const IniParser& parser, int section_number) {
  std::string section_number_str = base::IntToString(section_number);
  int history_length = parser.GetAs<int>(section_number_str, "max history", -1);
  std::string url_section = section_number_str + "history url";
  std::string title_section = section_number_str + "history title";
  std::string scrollpos_section = section_number_str + "history scrollpos list";
  if (history_length == 0) {
    // Empty tab without history, it happens. Add an empty history item for
    // consistency.
    current_history_ = 0;
    HistoryItem item;
    item.scroll_position_ = 0;
    history_.push_back(item);
    return true;
  }
  if (history_length == -1 ||
      history_length != parser.GetAs<int>(url_section, "count", -1) ||
      history_length != parser.GetAs<int>(title_section, "count", -1) ||
      history_length != parser.GetAs<int>(scrollpos_section, "count", -1)) {
    LOG(ERROR) << "Tab history count error for section " << section_number_str
               << ": url count: "
               << parser.Get(url_section, "count", "doesn't exist")
               << " : title count: "
               << parser.Get(title_section, "count", "doesn't exist")
               << " : scrollpos count: "
               << parser.Get(scrollpos_section, "count", "doesn't exist");
    return false;
  }
  /* current_history_ is an index within history_. It starts from 1 in the ini
   * and it should start from 0 in C++ - hence minus one */
  current_history_ =
      parser.GetAs<size_t>(
          base::IntToString(section_number), "current history", 1) - 1;

  // Don't import more history entries than Navigation Controller can take.
  size_t max_entry_count = content::kMaxSessionHistoryEntries - 1;
  // If there are too many history entries, start importing from a later index.
  int start_from_entry =
      std::max(0, static_cast<int>(history_length - max_entry_count));
  // Decrease current_history index by the amount of entries we skipped, make
  // sure it's still >= 0.
  current_history_ =
      std::max(0, static_cast<int>(current_history_) - start_from_entry);

  const size_t initial_current_history = current_history_;
  for (int i = start_from_entry; i < history_length; ++i) {
    HistoryItem item;
    item.url_ = GURL(parser.Get(url_section, base::IntToString(i), ""));
    item.scroll_position_
        = parser.GetAs<int>(scrollpos_section, base::IntToString(i), -1);
    item.title_
        = base::UTF8ToUTF16(
            parser.Get(title_section, base::IntToString(i), ""));
    if (!item.url_.is_valid() || item.scroll_position_ == -1)
      return false;
    // Remove adjacent duplicates which webkit doesn't like DNA-4206
    if (history_.empty() || history_.back().url_ != item.url_)
      history_.push_back(item);
    else if (initial_current_history + start_from_entry >= (size_t)i)
      current_history_--;
  }
  DCHECK(current_history_ < history_.size());
  return true;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
