// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/bookmark_importer.h"

#include <cstdlib>

#include "base/guid.h"
#include "base/logging.h"
#include "base/strings/string_util.h"

namespace {
std::string GetValidGUID(const std::string input) {
  if (base::IsValidGUID(input))
    return input;

  std::string guid;
  base::RemoveChars(input, "-", &guid);

  if (guid.length() == 32) {
    guid.insert(8, "-");
    guid.insert(13, "-");
    guid.insert(18, "-");
    guid.insert(23, "-");
  }
  return base::IsValidGUID(guid) ? guid : base::GenerateGUID();
}
}

namespace opera {
namespace common {
namespace migration {

BookmarkEntry::BookmarkEntry()
    : type(BOOKMARK_ENTRY_SEPARATOR),
      create_time(0),
      personal_bar_pos(-1),
      panel_pos(-1),
      trash_folder(false) {
}

BookmarkEntry::BookmarkEntry(const BookmarkEntry&) = default;

BookmarkEntry::~BookmarkEntry() {
}

BookmarkImporter::BookmarkImporter(
    scoped_refptr<BookmarkReceiver> bookmarkReceiver)
    : bookmark_receiver_(bookmarkReceiver) {
}

BookmarkImporter::~BookmarkImporter() {
}

bool BookmarkImporter::Parse(std::istream* input) {
  int newline_count = 0;
  int character;
  while ((character = input->get()) != '#') {
    if (input->eof())
      return true;
    else if (character == '\n') {
      if (newline_count == 3)
        return false;
      else
        newline_count++;
    }
  }

  input->putback('#');

  return ParseFolder(&entries_, input);
}

bool BookmarkImporter::ParseFolder(BookmarkEntries* entries,
                                   std::istream* input) {
  std::string line;
  BookmarkEntry current_entry;
  bool has_entry = false;
  bool folder_opened = false;

  while (std::getline(*input, line)) {
    // Mac and UNIX will fail to parse a file made on Windows unless we remove
    // the trailng '\r' on otherwise empty lines
    if (!line.empty() && line[0] == '\r')
      line.clear();

    if (line.size() == 0) {
      if (has_entry) {
        if (folder_opened) {
          if (!ParseFolder(&current_entry.entries, input))
            return false;
          folder_opened = false;
        } else {
          entries->push_back(current_entry);
          has_entry = false;
        }
      }
    } else if (line[0] == '#') {
      if (has_entry)
        return false;

      current_entry = BookmarkEntry();
      has_entry = true;

      switch (line[1]) {
      case 'U':
        current_entry.type = BookmarkEntry::BOOKMARK_ENTRY_URL;
        break;
      case 'S':
        current_entry.type = BookmarkEntry::BOOKMARK_ENTRY_SEPARATOR;
        break;
      case 'F':
        current_entry.type = BookmarkEntry::BOOKMARK_ENTRY_FOLDER;
        if (folder_opened)
          return false;
        else
          folder_opened = true;
        break;
      case 'D':
        current_entry.type = BookmarkEntry::BOOKMARK_ENTRY_DELETED;
        has_entry = false;  // Ignore this entry.
        break;
      default:
        NOTREACHED();
        has_entry = false;
        break;
      }
    } else if (line[0] == '-') {
      return true;
    } else {
      if (line[0] != '\t')
        return false;

      size_t separator_pos = line.find_first_of('=');

      if (separator_pos < 2 || separator_pos == line.npos)
        return false;

      std::string key = line.substr(1, separator_pos - 1);
      std::string value = line.substr(separator_pos + 1);

      if (key == "NAME")
        current_entry.name = value;
      else if (key == "UNIQUEID")
        current_entry.guid = GetValidGUID(value);
      else if (key == "URL")
        current_entry.url = value;
      else if (key == "DISPLAY URL")
        current_entry.display_url = value;
      else if (key == "CREATED")
        current_entry.create_time = std::atol(value.c_str());
      else if (key == "SHORT NAME")
        current_entry.short_name = value;
      else if (key == "DESCRIPTION")
        current_entry.description = value;
      else if (key == "PERSONALBAR_POS")
        current_entry.personal_bar_pos = std::atoi(value.c_str());
      else if (key == "PANEL_POS")
        current_entry.panel_pos = std::atoi(value.c_str());
      else if (key == "TRASH FOLDER")
        current_entry.trash_folder = value == "YES";
      else if (key == "PARTNERID")
        current_entry.partner_id = value;
    }
  }

  // Flush possibly pending entry.
  if (has_entry)
    entries->push_back(current_entry);

  return input->eof();
}

void BookmarkImporter::Import(std::unique_ptr<std::istream> input,
                              scoped_refptr<MigrationResultListener> listener) {
  bool result = Parse(input.get());

  if (result && bookmark_receiver_) {
    bookmark_receiver_->OnNewBookmarks(entries_, listener);
    return;
  }
  listener->OnImportFinished(opera::BOOKMARKS, result);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
