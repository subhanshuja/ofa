// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/prefs_importer.h"

namespace opera {
namespace common {
namespace migration {

PrefsImporter::PrefsImporter(scoped_refptr<PrefsListener> listener)
    : listener_(listener) {
}

PrefsImporter::~PrefsImporter() {
}

void PrefsImporter::Import(std::unique_ptr<std::istream> input,
                           scoped_refptr<MigrationResultListener> listener) {
  IniParser parser;
  bool success = parser.Parse(input.get());
  if (listener_) {
    if (success) {
      listener_->OnPrefsArrived(parser);
    }
    listener->OnImportFinished(opera::PREFS, success);
  }
}

}  // namespace migration
}  // namespace common
}  // namespace opera
