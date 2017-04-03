// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_VISITED_LINKS_IMPORTER_H_
#define COMMON_MIGRATION_VISITED_LINKS_IMPORTER_H_
#include "common/migration/importer.h"
#include "common/migration/vlinks/visited_links_listener.h"
#include "base/compiler_specific.h"

namespace opera {
namespace common {
namespace migration {

class VisitedLinksImporter : public Importer {
 public:
  explicit VisitedLinksImporter(scoped_refptr<VisitedLinksListener> listener);

  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~VisitedLinksImporter() override;
  bool Parse(std::istream* input);
  scoped_refptr<VisitedLinksListener> listener_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera


#endif  // COMMON_MIGRATION_VISITED_LINKS_IMPORTER_H_
