// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_SESSIONS_PARENT_WINDOW_H__
#define COMMON_MIGRATION_SESSIONS_PARENT_WINDOW_H__
#include <vector>

#include "common/migration/sessions/tab_session.h"

namespace opera {
namespace common {
class IniParser;
namespace migration {

/** Represents a window, which contains tabs.
 */
class ParentWindow {
 public:
  ParentWindow();
  ParentWindow(const ParentWindow&);
  ~ParentWindow();
  bool SetFromIni(const IniParser& parser, int section_number);

  std::vector<TabSession> owned_tabs_;  ///< Tabs that belong to this window.
  int x_;  ///< Horizontal offset relative to sceen origin.
  int y_;  ///< Vertical offset relative to sceen origin.
  int width_;  ///< Width of the window, without OS decorations.
  int height_;  ///< Height of the window, without OS decorations.

  /** State of the window. If other than STATE_RESTORED, ignore x, y, width and
   * height.
   */
  enum State {
    STATE_RESTORED,
    STATE_MINIMIZED,
    STATE_MAXIMIZED,
    STATE_FULLSCREEN,
    STATE_ERROR
  } state_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_SESSIONS_PARENT_WINDOW_H_
