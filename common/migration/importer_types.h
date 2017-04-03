// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_IMPORTER_TYPES_H_
#define COMMON_MIGRATION_IMPORTER_TYPES_H_

namespace opera {
enum ImporterType {
  BOOKMARKS,
  COOKIES,
  DOWNLOADS,
  EXTENSIONS,
  HISTORY,
  NOTES,
  PASSWORDS,
  PREFS,
  SEARCH_ENGINES,
  SEARCH_HISTORY,
  SESSIONS,
  SPEED_DIAL,
  TYPED_HISTORY,
  VISITED_LINKS,
  WEB_STORAGE,
  MOCK_FOR_TESTING,
  IMPORTER_ITEM_MAX,
};
}  // namespace opera

#endif  // COMMON_MIGRATION_IMPORTER_TYPES_H_
