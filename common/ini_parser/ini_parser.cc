// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <algorithm>
#include <sstream>
#include <utility>
#include <vector>

#include "common/ini_parser/ini_parser.h"
#include "base/logging.h"
#include "base/strings/string_util.h"

namespace opera {
namespace common {

namespace {

std::string Strip(const std::string& in) {
  std::string result = in;
  result.erase(result.find_last_not_of(" \n\r\t") + 1);  // Right strip
  result.erase(0, result.find_first_not_of(" \n\r\t"));  // Left strip
  return result;
}

std::pair<std::string, std::string> SplitOnFirstDelimiter(
    const std::string& line,
    const char delimiter) {
  size_t delimiter_pos = line.find_first_of(delimiter);
  std::string key;
  std::string value;
  if (delimiter_pos == line.npos) {
    key = line;
  } else {
    key = line.substr(0, delimiter_pos);
    value = line.substr(delimiter_pos + 1);
  }

  key = base::ToLowerASCII(Strip(key));
  return std::make_pair(key,
                        Strip(value));
}

/** Extracts section name from input line.
 * @param line input in the form "[SectionName]".
 * @param section_name[out] output in the form "SectionName"
 * @returns whether parsing was successful.
 */
bool SetSectionName(const std::string& line,
                    std::string& section_name) {
  size_t closing_brace_pos = line.rfind(']');
  if (closing_brace_pos == line.npos) {
    LOG(ERROR) << "Closing brace not found in line '" << line << "'";
    return false;
  }
  section_name =
      base::ToLowerASCII(line.substr(1, closing_brace_pos - 1));

  return true;
}
}  // namespace

IniParser::IniParser() {
}

IniParser::IniParser(const IniParser&) = default;

IniParser::~IniParser() {
}

bool IniParser::Parse(const std::string& input) {
  std::stringstream ss(input);
  return Parse(&ss);
}

bool IniParser::Parse(std::istream* input) {
  sections_.clear();

  const char key_value_separator = '=';
  const char comment_symbol = ';';
  const std::string opera_preferences_line = "\xEF\xBB\xBFOpera Preferences";


  std::string line;
  std::string current_section_name;
  KeyValueMap current_section_content;
  while (std::getline(*input, line)) {
    line = Strip(line);
    if (line.empty() || line[0] == comment_symbol)
      continue;  // Skip empty lines and comments
    if (line.substr(0, opera_preferences_line.size()) == opera_preferences_line)
      continue;  // Skip opera's non-parseable first line
    if (line[0] == '[') {
      // New section starts
      // Save section we've been parsing up to this point
      sections_[current_section_name] = current_section_content;
      // Start new section
      current_section_content.clear();
      if (!SetSectionName(line, current_section_name)) {
        LOG(ERROR) << "Could not set section name";
        return false;
      }

    } else {
      // This must be a key=value line
      std::pair<std::string, std::string> key_value_pair =
          SplitOnFirstDelimiter(line, key_value_separator);
      current_section_content.insert(key_value_pair);
    }
  }
  sections_[current_section_name] = current_section_content;
  return true;
}

bool IniParser::HasValue(const std::string& section,
                         const std::string& key) const {
  std::string sec_internal = base::ToLowerASCII(section);
  std::string key_internal = base::ToLowerASCII(key);

  SectionsMap::const_iterator section_iterator = sections_.find(sec_internal);
  if (section_iterator == sections_.end())
    return false;  // There's no such section
  const KeyValueMap& section_values = section_iterator->second;
  KeyValueMap::const_iterator value_iterator = section_values.find(key_internal);
  return value_iterator != section_values.end();
}

bool IniParser::HasSection(const std::string& section) const {
  std::string sec_internal = base::ToLowerASCII(section);
  return sections_.find(sec_internal) != sections_.end();
}

std::string IniParser::Get(const std::string& section,
                const std::string& key,
                const std::string& defaultValue) const {
  std::string sec_internal = base::ToLowerASCII(section);
  std::string key_internal = base::ToLowerASCII(key);

  SectionsMap::const_iterator section_iterator = sections_.find(sec_internal);
  if (section_iterator == sections_.end())
    return defaultValue;  // There's no such section
  const KeyValueMap& section_values = section_iterator->second;
  KeyValueMap::const_iterator value_iterator = section_values.find(key_internal);
  if (value_iterator == section_values.end())
    return defaultValue;  // No such key in section
  return value_iterator->second;
}

template<>
bool IniParser::GetAs<bool>(const std::string& section,
                            const std::string& key,
                            const bool& default_value) const {
  if (!HasValue(section, key))
    return default_value;
  std::string value = base::ToLowerASCII(Get(section, key, ""));
  if (value == "true" || value == "on" || value == "enabled" || value == "1")
    return true;
  if (value == "false" || value == "off" || value == "disabled" || value == "0")
    return false;
  return default_value;
}
}  // namespace common
}  // namespace opera
