// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/speed_dial_importer.h"

#include "base/guid.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

#include "common/ini_parser/ini_parser.h"

namespace opera {
namespace common {
namespace migration {

SpeedDialListener::SpeedDialEntry::SpeedDialEntry()
  : title_is_custom_(false),
    enable_reload_(false),
    reload_interval_(0),
    reload_only_if_expired_(false) {
}

SpeedDialListener::SpeedDialEntry::SpeedDialEntry(
    const SpeedDialListener::SpeedDialEntry&) = default;

SpeedDialListener::SpeedDialEntry::~SpeedDialEntry() {
}

SpeedDialListener::BackgroundInfo::BackgroundInfo()
  : enabled_(false) {
}

SpeedDialListener::BackgroundInfo::~BackgroundInfo() {
}

SpeedDialImporter::SpeedDialImporter(scoped_refptr<SpeedDialListener> listener)
    : listener_(listener) {
}

SpeedDialImporter::~SpeedDialImporter() {
}

void SpeedDialImporter::Import(
    std::unique_ptr<std::istream> input,
    scoped_refptr<MigrationResultListener> listener) {
  listener->OnImportFinished(opera::SPEED_DIAL,
                             ImportImplementation(input.get()));
}

bool SpeedDialImporter::ImportImplementation(std::istream* input) {
  IniParser parser;
  if (!parser.Parse(input)) {
    LOG(ERROR) << "Could not parse input as ini";
    return false;
  }

  int section_id = 1;
  std::vector<SpeedDialListener::SpeedDialEntry> entries;
  for (std::string section_prefix = "Speed Dial ";
       parser.HasValue(section_prefix + base::IntToString(section_id), "Title");
       ++section_id) {
    std::string section_name = section_prefix + base::IntToString(section_id);
    SpeedDialListener::SpeedDialEntry entry;
    entry.display_url_ =
        GURL(parser.Get(section_name, "Display Url", ""));
    entry.enable_reload_ =
        parser.GetAs<bool>(section_name, "Reload Policy", false);
    entry.partner_id_ =
        parser.Get(section_name, "Partner ID", "");
    entry.reload_interval_ =
        parser.GetAs<int>(section_name, "Reload Interval", -1);
    entry.reload_only_if_expired_ =
        parser.GetAs<bool>(section_name, "Reload Only If Expired", false);
    entry.title_ =
        parser.Get(section_name, "Title", "");
    entry.title_is_custom_ =
        parser.GetAs<bool>(section_name, "Custom Title", false);
    entry.url_ = GURL(parser.Get(section_name, "Url", ""));
    entry.extension_id_ = parser.Get(section_name, "Extension ID", "");
    entries.push_back(entry);
  }

  SpeedDialListener::BackgroundInfo background;
  background.enabled_ = parser.GetAs<bool>("Background", "Enabled", false);
  background.file_path_ = parser.Get("Background", "Filename", "");
  background.layout_ = parser.Get("Background", "Layout", "");

  if (listener_)
    listener_->OnSpeedDialParsed(entries, background);
  return true;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
