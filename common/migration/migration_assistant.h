// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_MIGRATION_ASSISTANT_H_
#define COMMON_MIGRATION_MIGRATION_ASSISTANT_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"

#include "common/migration/importer.h"
#include "common/migration/migration_result_listener.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace opera {
namespace common {
namespace migration {

class ProfileFolderFinder;

/** Provides a way to register various Importer objects with their names
 * and then call them.
 *
 * Note that you don't have to use the assistant. It is legal to instantiate
 * and call concrete importers where needed.
 */
class MigrationAssistant
    : public base::RefCountedThreadSafe<MigrationAssistant> {
 public:
  /** Creates a MigrationAssistant.
   *
   * @param task_runner SingleThreadTaskRunner for the thread which will do the
   * work. Work consists of opening/reading a file and parsing it. Should
   * generally be the result of
   * BrowserThread::GetMessageLoopProxyForThread(FILE). May not be null.
   * @param profile_folder_finder will be used to find folders for the
   * registered input files. MigrationAssistant does not take ownership of this
   * object.
   */
  MigrationAssistant(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      const ProfileFolderFinder* profile_folder_finder);

  /** Registers an Importer to respond for a particular type and work
   * with a particular input file.
   * @param importer the importer to register. Takes ownership (obviously,
   * since this is a unique_ptr).
   * @param item_type the definition under which the importer will be accessible
   * through Import().
   * @param input_file_path path to where the data is. The ProfileFolderFinder
   * given at construction will be used to obtain the path to the profile
   * folder and @a input_file_path will be appended. The resulting file will
   * become the input to the importer.
   * @param open_in_binary_mode whether the file should be open in binary mode
   */
  void RegisterImporter(scoped_refptr<Importer> importer,
                        opera::ImporterType item_type,
                        base::FilePath input_file_path,
                        bool open_in_binary_mode);
  /** Calls the Importer registered under @a item_type.
   *
   * Asynchronously opens the file associated with this Importer (set during
   * RegisterImporter), provides it to the Importer and executes
   * Importer::Import(). Results (ie. return value of Importer::Import()) will
   * be communicated through the @a listener.
   * @param import_name Name of the importer, as given during RegisterImporter().
   * @param listener to be notified of the import results.
   */
  void ImportAsync(opera::ImporterType item_type,
                   scoped_refptr<MigrationResultListener> listener);

 protected:
  virtual ~MigrationAssistant();
  friend class base::RefCountedThreadSafe<MigrationAssistant>;

 private:
  void ImportClosure(opera::ImporterType item_type,
                     scoped_refptr<MigrationResultListener> listener);

  struct ImporterBundle {
    ImporterBundle();
    ImporterBundle(const ImporterBundle&);
    ~ImporterBundle();
    scoped_refptr<Importer> importer;
    base::FilePath input_file_path;
    bool open_in_binary_mode;
  };

  typedef std::map<opera::ImporterType, ImporterBundle> ImportersMap;
  ImportersMap importers_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  const ProfileFolderFinder* profile_folder_finder_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera


#endif  // COMMON_MIGRATION_MIGRATION_ASSISTANT_H_
