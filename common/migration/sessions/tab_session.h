// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_MIGRATION_SESSIONS_TAB_SESSION_H_
#define COMMON_MIGRATION_SESSIONS_TAB_SESSION_H_

#include <string>
#include <vector>

#include "url/gurl.h"

namespace opera {
namespace common {
class IniParser;
namespace migration {

/** Represents a single tab, with its history and properties.
 */
class TabSession {
 public:
  TabSession();
  TabSession(const TabSession&);
  ~TabSession();
  bool SetFromIni(const IniParser& parser, int section_number);

  enum Type {
    BROWSER_WINDOW,
    DOCUMENT_WINDOW,
    MAIL_WINDOW,
    MAIL_COMPOSE_WINDOW,
    TRANSFER_WINDOW,
    HOTLIST_WINDOW,
    CHAT_WINDOW,
    PANEL_WINDOW,
    GADGET_WINDOW,
    DEVTOOLS_WINDOW,
    UNKNOWN
  } type_;

  // Non-empty if type_ == PANEL_WINDOW. May be one of the following:
  // "Bookmarks", "Mail", "Contacts", "History", "Transfers", "Links",
  // "Windows", "Chat", "Info", "Widgets", "Notes", "Music", "Start", "Search",
  // "Unite", "Extensions"
  std::string panel_type_;

  struct HistoryItem {
    GURL url_;  ///< URL of the page.
    base::string16 title_;  ///< Title of the page.
    int scroll_position_;  ///< Vertical scroll offset.
  };


  std::vector<HistoryItem> history_;  ///< Last element is the newest.
  size_t current_history_;  ///< Index within history_ of the current page
  bool active_;  ///< If true, this is the selected tab.
  int tab_index_;  ///< Index on the tab list, less is more to the left.
  int stack_index_;  ///< Index on the stack (RMB + scroll), less is higher.
  int x_;  ///< Horizontal offset within the parent window.
  int y_;  ///< Vertical offset within the parent window.
  /* Note, width_ and height_ may be smaller than width and height of the parent
   * window as they don't include various bars and stuff. */
  int width_;  ///< Width of the canvas.
  int height_;  ///< Height of the canvas.
  bool show_scrollbars_;
  int scale_;  ///< Scaling factor, in percent (100 is no scaling)
  std::string encoding_;  ///< Encoding name (ex. 'utf-8'), empty means auto

  int group_id_;  ///< ID of tab group this belongs to. 0 = not member of group.
  bool group_active_;  ///< Whether is the active tab within the group,
  bool hidden_;  ///< Whether is hidden, ex. as a non-active group member

 private:
  bool ParseHistory(const IniParser& parser, int section_number);
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_SESSIONS_TAB_SESSION_H_
