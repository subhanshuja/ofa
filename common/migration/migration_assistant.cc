// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/migration_assistant.h"

#include <fstream>

#include "base/bind.h"
#include "base/logging.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"

#include "common/migration/profile_folder_finder.h"

namespace opera {
namespace common {
namespace migration {

MigrationAssistant::MigrationAssistant(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const ProfileFolderFinder* profile_folder_finder)
  : task_runner_(task_runner),
    profile_folder_finder_(profile_folder_finder) {
}

MigrationAssistant::~MigrationAssistant() {
}

MigrationAssistant::ImporterBundle::ImporterBundle() {
}

MigrationAssistant::ImporterBundle::ImporterBundle(
    const MigrationAssistant::ImporterBundle&) = default;

MigrationAssistant::ImporterBundle::~ImporterBundle() {
}

void MigrationAssistant::RegisterImporter(scoped_refptr<Importer> importer,
                                          opera::ImporterType item_type,
                                          base::FilePath input_file_path,
                                          bool open_in_binary_mode) {
  ImporterBundle bundle;
  bundle.importer = importer;
  bundle.input_file_path = input_file_path;
  bundle.open_in_binary_mode = open_in_binary_mode;
  importers_[item_type] = bundle;
}

void MigrationAssistant::ImportAsync(
    opera::ImporterType item_type,
    scoped_refptr<MigrationResultListener> listener) {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&MigrationAssistant::ImportClosure,
                                    this,
                                    item_type,
                                    listener));
}

void MigrationAssistant::ImportClosure(
    opera::ImporterType item_type,
    scoped_refptr<MigrationResultListener> listener) {
  ImportersMap::iterator loader_iterator = importers_.find(item_type);
  if (loader_iterator == importers_.end()) {
    LOG(ERROR) << "There's no Importer registered for type: " << item_type;
    if (listener) {
      listener->OnImportFinished(item_type, false);
    }
    return;
  }
  const ImporterBundle& bundle = loader_iterator->second;
  // Try to open the input file
  base::FilePath path;
  if (!profile_folder_finder_->FindFolderFor(bundle.input_file_path, &path)) {
    LOG(ERROR) << "Couldn't find the folder which contains "
               << bundle.input_file_path.value();
    if (listener) {
      listener->OnImportFinished(item_type, false);
    }
    return;
  }
  path = path.Append(bundle.input_file_path);
  std::ios_base::openmode open_mode = std::ios_base::in;
  if (bundle.open_in_binary_mode)
    open_mode |= std::ios_base::binary;
  std::unique_ptr<std::ifstream> input_file(
      new std::ifstream(path.value().c_str(), open_mode));
  if (!input_file->is_open() || input_file->fail()) {
    LOG(ERROR) << "Could not open file: " << bundle.input_file_path.value();
    if (listener) {
      listener->OnImportFinished(item_type, false);
    }
    return;
  }
  bundle.importer->Import(std::move(input_file), listener);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
