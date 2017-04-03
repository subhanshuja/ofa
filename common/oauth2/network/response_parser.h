// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <map>
#include <string>

namespace base {
class DictionaryValue;
}

namespace opera {
namespace oauth2 {
class ResponseParser {
 public:
  enum Type { UNSET, STRING, INTEGER, BOOLEAN };

  enum Optional { IS_OPTIONAL, IS_REQUIRED };

  enum ParseType {
    // Any not expected element causes a parse error
    PARSE_STRICT,
    // Any not expected element is ignored
    PARSE_SOFT
  };

  ResponseParser();
  ~ResponseParser();

  void Expect(const std::string& key, Type type, Optional optional);
  bool Parse(base::DictionaryValue* dict, ParseType type);

  bool GetBoolean(const std::string& key) const;
  int GetInteger(const std::string& key) const;
  std::string GetString(const std::string& key) const;

  bool HasBoolean(const std::string& key) const;
  bool HasInteger(const std::string& key) const;
  bool HasString(const std::string& key) const;

 protected:
  void Reset();

  std::map<std::string, std::pair<Type, Optional>> items_;
  std::map<std::string, std::string> str_values_;
  std::map<std::string, int> int_values_;
  std::map<std::string, bool> bool_values_;
};
}  // namespace oauth2
}  // namespace opera
