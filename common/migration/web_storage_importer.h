// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_MIGRATION_WEB_STORAGE_IMPORTER_H_
#define COMMON_MIGRATION_WEB_STORAGE_IMPORTER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/ref_counted.h"

#include "common/migration/importer.h"

struct _xmlNode;
typedef _xmlNode xmlNode;

namespace opera {
namespace common {
namespace migration {

class BulkFileReader;
class WebStorageListener;

/** Importer for web storage (aka local storage, DOM storage).
 *
 * Web storage can be found in the 'pstorage' folder within the profile folder.
 * It's organized as an index file (psindex.dat) and per-site files with
 * key-value pairs (UTF16 encoded with Base64). All files are XMLs.
 *
 * Importing is done by first parsing the index file (which yields
 * URL to storage file mappings), then parsing the storage files.
 */
class WebStorageImporter : public Importer {
 public:
  /** WebStorageImporter c-tor.
   * @param reader Will use this to open and read a bunch of files.
   * @param listener Will receive web storage data. You can use the default
   * WebStorageListener that puts the web storage data into DomStorage.
   * @param is_extension Is this importer working with extension specific
   * webstorage.
   */
  WebStorageImporter(scoped_refptr<BulkFileReader> reader,
                     scoped_refptr<WebStorageListener> listener,
                     bool is_extension);

  /** Parses the content of the index file and then contents of storage
   * files contained in the index file.
   * Will call the listener as input is parsed.
   * @param input Content of the psindex.dat file.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~WebStorageImporter() override;

 private:
  bool ImportImplementation(std::istream* input);
  typedef std::pair<std::string, std::string> FileUrlPair;

  /** Parse the content of the index file and populate files_to_parse_. */
  bool ReadInitialInput(std::istream* input);
  bool ParseSection(xmlNode* node);

  bool ReadDataFiles();

  /** Called for each entry in files_to_parse_, opens and parses the storage
   * file, calls listener_ after extracting key-value pairs. */
  bool ParseStorageFileContent(const FileUrlPair& data);
  bool ParseElementNode(xmlNode* element_node, const std::string& origin_url);
  scoped_refptr<BulkFileReader> reader_;
  scoped_refptr<WebStorageListener> listener_;
  typedef std::vector<FileUrlPair> FilesToParseVector;
  FilesToParseVector files_to_parse_;
  bool is_extension_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WEB_STORAGE_IMPORTER_H_
