// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_BROWSER_OP_SITE_PATCHER_H_
#define COMMON_SITEPATCHER_BROWSER_OP_SITE_PATCHER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/memory/ref_counted_delete_on_message_loop.h"
#include "base/memory/shared_memory.h"
#include "base/synchronization/lock.h"
#include "common/sitepatcher/browser/op_site_patcher_config.h"
#include "common/sitepatcher/browser/op_update_checker.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/persistent_pref_store.h"
#include "components/prefs/pref_store.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace content {
class RenderProcessHost;
}

class OpSitePatcher : public base::RefCountedDeleteOnMessageLoop<OpSitePatcher>,
                      public content::NotificationObserver,
                      public PrefStore::Observer {
 public:
  OpSitePatcher(scoped_refptr<net::URLRequestContextGetter> request_context,
                const OpSitepatcherConfig& config);

  /**
   * OpSitePatcher::Observer is notified about state changes OpSitePatcher.
   *
   * @see OpSitePatcher
   */
  class Observer {
   public:
    virtual ~Observer() {}

    /**
     * Called when the enable or disabled state changes for the site patcher.
     */
    virtual void OnStateChanged() = 0;
  };

  void Initialize();

  bool OnPrefsOverrideUpdated();
  bool OnBrowserJSUpdated();
  bool OnWebBluetoothBlacklistUpdated();
  bool LoadBrowserJS(bool load_new_browser_js);
  void LoadUserJS();
  bool LoadSitePrefs(bool load_new);
  bool LoadWebBluetoothBlacklist(bool load_new);

  void SetEnable(bool enable);
  bool enabled() { return config_.sitepatcher_enabled; }
  bool browser_js_loaded() { return browser_js_memory_.get() != NULL; }
  bool site_prefs_loaded() { return site_prefs_shared_memory_.get() != NULL; }

  // Inherited from NotificationObserver
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Inherited from PrefStore::Observer
  void OnPrefValueChanged(const std::string& key) override;
  void OnInitializationCompleted(bool succeeded) override;

  /**
   * Adds observer for receiving notification for state changes.
   *
   * @param observer the observer to be added.
   */
  void AddObserver(OpSitePatcher::Observer* observer);

  /**
   * Removes observer from the list of observers.
   *
   * @param observer the observer to be removed.
   */
  void RemoveObserver(OpSitePatcher::Observer* observer);

 private:
  ~OpSitePatcher() override;
  friend class base::RefCountedDeleteOnMessageLoop<OpSitePatcher>;
  friend class base::DeleteHelper<OpSitePatcher>;

  std::string LoadFile(base::FilePath path, bool load_new);
  bool CreateSharedMemory(const void* data,
                          size_t size,
                          std::unique_ptr<base::SharedMemory>& memory);

  // UI thread.
  void InitPrefs();
  void Start();
  void StartChecker(bool force_check);
  void BroadcastBrowserJS();
  void BroadcastUserJS();
  void SendBrowserJS(content::RenderProcessHost* process);
  void SendUserJS(content::RenderProcessHost* process);
  void SendEnable(content::RenderProcessHost* process);
  void BroadcastSitePrefs();
  void SendSitePrefs(content::RenderProcessHost* process);
  void OnFileLoaded(std::string pref_key, bool success);
  void BroadcastStateChanged();

  bool ShareMemory(base::SharedMemory* memory,
                   base::SharedMemoryHandle& memory_handle,
                   base::ProcessHandle process_handle);

  bool VerifySignature(const std::string& text);

  content::NotificationRegistrar registrar_;

  OpSitepatcherConfig config_;

  OpUpdateChecker update_checker_;

  scoped_refptr<JsonPrefStore> prefs_;

  // Contains the browser.js script.
  std::unique_ptr<base::SharedMemory> browser_js_memory_;

  // Contains the user.js script.
  std::unique_ptr<base::SharedMemory> user_js_memory_;

  // Contains the site prefs / settings overrides.
  std::unique_ptr<base::SharedMemory> site_prefs_shared_memory_;

  // Protects shared_memory_ and site_prefs_shared_memory_.
  base::Lock lock_;

  /**
   * List of observers for state changes
   */
  base::ObserverList<OpSitePatcher::Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(OpSitePatcher);
};

#endif  // COMMON_SITEPATCHER_BROWSER_OP_SITE_PATCHER_H_
