// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_TOOLS_INI_PARSER_H_
#define COMMON_MIGRATION_TOOLS_INI_PARSER_H_
#include <istream>
#include <string>
#include <sstream>
#include <map>

namespace opera {
namespace common {

class IniParser {
 public:
  IniParser();
  IniParser(const IniParser&);
  ~IniParser();

  /** Parse the input stream, which is assumed to be a valid ini file content.
   *
   * Parse() may be called multiple times on the same object. Results of
   * previous parsing will be dismissed.
   *
   * @returns false if there was an error during parsing (bad syntax), true if
   * everything went ok.
   */
  bool Parse(std::istream* input);
  bool Parse(const std::string& input);

  /** Check if a value was present in the input.
   *
   * @param section Section name, as found within square brackets. Empty string
   * for the top-level pseudo section.
   * @param key Name of the key within the section.
   * @returns whether the key is present.
   */
  bool HasValue(const std::string& section, const std::string& key) const;

  /** Check if a section was present in the input.
   *
   * @param section Section name, as found within square brackets. Empty string
   * for the top-level pseudo section.
   * @returns whether the section is present.
   */
  bool HasSection(const std::string& section) const;

  /** Get the value associated with the key.
   *
   * @param section Section name, as found within square brackets. Empty string
   * for the top-level pseudo section.
   * @param key Name of the key within the section.
   * @param default_value The value to return if the key is not found.
   * @returns value associated with @a key in @a section. The string will be
   * stripped of whitespace.
   */
  std::string Get(const std::string& section,
                  const std::string& key,
                  const std::string& default_value) const;

  /** Get the value associated with the key and convert it to an arbitrary type.
   *
   * Uses the streaming operator>> for conversion.
   *
   * @warning bool specialisation only works if the value is "0" or "1", not
   * "true" or "false". This is because the built-int operator>> for bool works
   * this way.
   *
   * @param section Section name, as found within square brackets. Empty string
   * for the top-level pseudo section.
   * @param key Name of the key within the section.
   * @param default_value The value to return if the key is not found or was
   * not convertible to T.
   */
  template<typename T>
  T GetAs(const std::string& section,
          const std::string& key,
          const T& default_value) const {
    if (!HasValue(section, key))
      return default_value;
    T result = T();
    std::istringstream stream(Get(section, key, ""));
    stream >> result;
    return stream.fail() ? default_value : result;
  }

 private:
  typedef std::map<std::string, std::string> KeyValueMap;
  typedef std::map<std::string, KeyValueMap> SectionsMap;
  SectionsMap sections_;
};


// bool has a specialization that works with "true"/"false", "on"/off",
// "enabled"/"disabled"
template<>
bool IniParser::GetAs<bool>(const std::string& section,
                            const std::string& key,
                            const bool& default_value) const;
}  // namespace common
}  // namespace opera


#endif  // COMMON_MIGRATION_TOOLS_INI_PARSER_H_
