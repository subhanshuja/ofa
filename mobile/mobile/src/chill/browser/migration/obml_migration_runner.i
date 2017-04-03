// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/migration/obml_migration_runner.h"
%}

// Generate swig interface for OBMLFontInfo
%include "src/chill/browser/migration/obml_font_info.h"

namespace opera {

class OBMLMigrationRunner {
 public:
  static void StartMigration(std::vector<OBMLFontInfo> font_info,
                             base::Callback<void(const OpArguments&)> callback);

 private:
  OBMLMigrator();
};

}  // namespace opera

%template(OBMLFontInfoVector) std::vector<opera::OBMLFontInfo>;
