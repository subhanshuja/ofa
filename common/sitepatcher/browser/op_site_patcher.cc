// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/browser/op_site_patcher.h"

#include <string>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/shared_memory.h"
#include "base/memory/singleton.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "common/crypto/op_verify_signature.h"
#include "common/message_generator/all_messages.h"
#include "common/sitepatcher/browser/browserjs_key.h"
#include "common/sitepatcher/browser/op_site_patcher_config.h"
#include "common/sitepatcher/config.h"
#include "common/sitepatcher/sitepatcher_cert_config.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "content/browser/bluetooth/bluetooth_blacklist.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"

namespace {

const char kDisableSitePatcher[] = "disable-site-patcher";

class ReadErrorHandler : public PersistentPrefStore::ReadErrorDelegate {
 public:
  ReadErrorHandler(base::Callback<void(PersistentPrefStore::PrefReadError)> cb)
      : callback_(cb) {}

  void OnError(PersistentPrefStore::PrefReadError error) override {
    callback_.Run(error);
  }

 private:
  base::Callback<void(PersistentPrefStore::PrefReadError)> callback_;
};

void HandleReadError(PersistentPrefStore::PrefReadError error) {
  if (error != PersistentPrefStore::PREF_READ_ERROR_FILE_NOT_SPECIFIED)
    DLOG(INFO) << "Pref read error = " << error;
}

}  // namespace

using content::BluetoothBlacklist;
using content::BrowserThread;
using content::RenderProcessHost;

#define GET_BOOL(key, dest) \
  (prefs_->GetValue(key, &value) && value->GetAsBoolean(&dest))

OpSitePatcher::OpSitePatcher(
    scoped_refptr<net::URLRequestContextGetter> request_context,
    const OpSitepatcherConfig& config)
    : base::RefCountedDeleteOnMessageLoop<OpSitePatcher>(
          base::ThreadTaskRunnerHandle::Get()),
      config_(config),
      update_checker_(request_context, config_, this) {}

OpSitePatcher::~OpSitePatcher() {
  prefs_->RemoveObserver(this);
}

void OpSitePatcher::Initialize() {
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_CREATED,
                 content::NotificationService::AllSources());

  InitPrefs();
}

bool OpSitePatcher::OnPrefsOverrideUpdated() {
  return LoadSitePrefs(true);
}

bool OpSitePatcher::OnBrowserJSUpdated() {
  return LoadBrowserJS(true);
}

bool OpSitePatcher::OnWebBluetoothBlacklistUpdated() {
  return LoadWebBluetoothBlacklist(true);
}

void OpSitePatcher::Observe(int type,
                            const content::NotificationSource& source,
                            const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_RENDERER_PROCESS_CREATED: {
      RenderProcessHost* process =
          content::Source<RenderProcessHost>(source).ptr();

      SendEnable(process);

      if (browser_js_memory_.get()) {
        SendBrowserJS(process);
      }
      if (user_js_memory_.get()) {
        SendUserJS(process);
      }
      if (site_prefs_shared_memory_.get()) {
        SendSitePrefs(process);
      }
      break;
    }
    default:
      DCHECK(false);
  }
}

void OpSitePatcher::OnPrefValueChanged(const std::string& /*key*/) {}

void OpSitePatcher::OnInitializationCompleted(bool succeeded) {
  if (succeeded)
    Start();
}

void OpSitePatcher::InitPrefs() {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      JsonPrefStore::GetTaskRunnerForFile(config_.update_prefs_file,
                                          BrowserThread::GetBlockingPool());
  prefs_ = new JsonPrefStore(config_.update_prefs_file, task_runner,
                             std::unique_ptr<PrefFilter>());
  prefs_->AddObserver(this);

  // Guarantee that initialization happens after this function returned.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&PersistentPrefStore::ReadPrefsAsync, prefs_.get(),
                 new ReadErrorHandler(base::Bind(&HandleReadError))));

  update_checker_.SetPrefs(prefs_.get());
}

void OpSitePatcher::Start() {
  DCHECK(prefs_->IsInitializationComplete());

  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
                          base::Bind(&OpSitePatcher::LoadUserJS, this));

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kDisableSitePatcher)) {
    config_.sitepatcher_enabled = false;
    BroadcastStateChanged();
    DLOG(INFO) << "BrowserJS and site prefs disabled.";
  }

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&OpSitePatcher::LoadBrowserJS, this, false),
      base::Bind(&OpSitePatcher::OnFileLoaded, this, PREF_BROWSER_JS_LOCAL));

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&OpSitePatcher::LoadSitePrefs, this, false),
      base::Bind(&OpSitePatcher::OnFileLoaded, this,
                 PREF_PREFS_OVERRIDE_LOCAL));

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&OpSitePatcher::LoadWebBluetoothBlacklist, this, false),
      base::Bind(&OpSitePatcher::OnFileLoaded, this,
                 PREF_WEB_BLUETOOTH_BLACKLIST_LOCAL));
}

void OpSitePatcher::StartChecker(bool force_check) {
  update_checker_.Start(force_check);
}

void OpSitePatcher::OnFileLoaded(std::string pref_key, bool success) {
  if (!success) {
    // Clear the local timestamp to force a fresh download.
    prefs_->SetValue(pref_key,
                     base::WrapUnique(new base::FundamentalValue(0.0)),
                     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&OpSitePatcher::StartChecker, this, !success));
}

bool OpSitePatcher::LoadBrowserJS(bool load_new_browser_js) {
  std::string browser_js =
      LoadFile(config_.browser_js_file, load_new_browser_js);

  if (browser_js.empty())
    return false;

  if (config_.verify_browser_js) {
    if (!VerifySignature(browser_js)) {
      DLOG(INFO) << "Signature check failed for browser js.";
      // Delete browser.js to force a fresh download.
      base::DeleteFile(config_.browser_js_file, false);
      return false;
    }
  }

  base::string16 web_string = base::UTF8ToUTF16(browser_js);
  size_t size = web_string.length() * sizeof(base::char16);

  if (!CreateSharedMemory(web_string.data(), size, browser_js_memory_))
    return true;  // Don't report the error becasuse we have no way to recover.

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&OpSitePatcher::BroadcastBrowserJS, this));

  return true;
}

bool OpSitePatcher::LoadWebBluetoothBlacklist(bool load_new) {
  std::string blacklist =
      LoadFile(config_.web_bluetooth_blacklist_file, load_new);

  if (blacklist.empty())
    return false;

  if (config_.verify_web_bluetooth_blacklist) {
    if (!VerifySignature(blacklist)) {
      DLOG(INFO) << "Signature check failed for Web Bluetooth blacklist.";
      return false;
    }
  }

  // Remove the signature now that it is verified.
  const size_t sig_end = blacklist.find('\n');
  if (blacklist[0] == '/' && blacklist[1] == '/' &&
      sig_end != std::string::npos && blacklist.length() > sig_end + 1)
    blacklist.erase(0, sig_end + 1);

  BluetoothBlacklist::Get().Add(blacklist);

  return true;
}

void OpSitePatcher::LoadUserJS() {
  std::string user_js = LoadFile(config_.user_js_file, false);

  if (user_js.empty())
    return;

  base::string16 web_string = base::UTF8ToUTF16(user_js);
  size_t size = web_string.length() * sizeof(base::char16);

  if (!CreateSharedMemory(web_string.data(), size, user_js_memory_))
    return;  // Don't report the error becasuse we have no way to recover.

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&OpSitePatcher::BroadcastUserJS, this));
}

void OpSitePatcher::BroadcastBrowserJS() {
  RenderProcessHost::iterator i(RenderProcessHost::AllHostsIterator());
  for (; !i.IsAtEnd(); i.Advance())
    SendBrowserJS(i.GetCurrentValue());
}

void OpSitePatcher::BroadcastUserJS() {
  RenderProcessHost::iterator i(RenderProcessHost::AllHostsIterator());
  for (; !i.IsAtEnd(); i.Advance())
    SendUserJS(i.GetCurrentValue());
}

void OpSitePatcher::SendBrowserJS(RenderProcessHost* process) {
  base::SharedMemoryHandle memory_handle;
  if (ShareMemory(browser_js_memory_.get(), memory_handle,
                  process->GetHandle())) {
    process->Send(new OpMsg_BrowserJsUpdated(
        memory_handle, browser_js_memory_->requested_size()));
  }
}

void OpSitePatcher::SendUserJS(RenderProcessHost* process) {
  base::SharedMemoryHandle memory_handle;
  if (ShareMemory(user_js_memory_.get(), memory_handle, process->GetHandle())) {
    process->Send(
        new OpMsg_SetUserJs(memory_handle, user_js_memory_->requested_size()));
  }
}

bool OpSitePatcher::LoadSitePrefs(bool load_new) {
  std::string site_prefs = LoadFile(config_.prefs_override_file, load_new);
  if (site_prefs.empty())
    return false;

  if (config_.verify_site_prefs) {
    if (!VerifySignature(site_prefs)) {
      DLOG(INFO) << "Signature check failed for site prefs.";
      return false;
    }
  }

  size_t size = site_prefs.size() * sizeof(std::string::value_type);

  if (!CreateSharedMemory(site_prefs.data(), size, site_prefs_shared_memory_))
    return true;  // Don't report the error becasuse we have no way to recover.

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&OpSitePatcher::BroadcastSitePrefs, this));

  return true;
}

void OpSitePatcher::BroadcastSitePrefs() {
  RenderProcessHost::iterator i(RenderProcessHost::AllHostsIterator());
  for (; !i.IsAtEnd(); i.Advance())
    SendSitePrefs(i.GetCurrentValue());
}

void OpSitePatcher::SendSitePrefs(RenderProcessHost* process) {
  base::SharedMemoryHandle memory_handle;
  if (ShareMemory(site_prefs_shared_memory_.get(), memory_handle,
                  process->GetHandle()))
    process->Send(new OpMsg_SitePrefsUpdated(
        memory_handle, site_prefs_shared_memory_->requested_size()));
}

std::string OpSitePatcher::LoadFile(base::FilePath path, bool load_new) {
  std::string content;
  base::FilePath new_path(path);

  if (load_new)
    new_path = new_path.AddExtension(FILE_PATH_LITERAL(".new"));

  if (!base::ReadFileToString(new_path, &content))
    return "";

  if (load_new)
    base::ReplaceFile(new_path, path, NULL);

  return content;
}

bool OpSitePatcher::CreateSharedMemory(
    const void* data,
    size_t size,
    std::unique_ptr<base::SharedMemory>& memory) {
  std::unique_ptr<base::SharedMemory> new_mem(new base::SharedMemory());

  if (!new_mem->CreateAndMapAnonymous(size))
    return false;

  memcpy(new_mem->memory(), data, size);

  lock_.Acquire();
  memory.swap(new_mem);
  lock_.Release();

  return true;
}

bool OpSitePatcher::ShareMemory(base::SharedMemory* memory,
                                base::SharedMemoryHandle& memory_handle,
                                base::ProcessHandle process_handle) {
  base::AutoLock lock(lock_);

  DCHECK(memory);

  if (!process_handle)
    return false;

  if (!memory->ShareToProcess(process_handle, &memory_handle))
    return false;

  return base::SharedMemory::IsHandleValid(memory_handle);
}

bool OpSitePatcher::VerifySignature(const std::string& text) {
#if BUILDFLAG(USE_SITEPATCHER_TEST_CERTIFICATE)
  return OpVerifySignature(text, "// ", DOM_BROWSERJS_KEY_TEST,
                           sizeof(DOM_BROWSERJS_KEY_TEST));
#else
  return OpVerifySignature(text, "// ", DOM_BROWSERJS_KEY_1,
                           sizeof(DOM_BROWSERJS_KEY_1)) ||
         OpVerifySignature(text, "// ", DOM_BROWSERJS_KEY_2,
                           sizeof(DOM_BROWSERJS_KEY_2));
#endif  // USE_SITEPATCHER_TEST_CERTIFICATE
}

void OpSitePatcher::SetEnable(bool enable) {
  if (enable != config_.sitepatcher_enabled) {
    config_.sitepatcher_enabled = enable;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&OpSitePatcher::BroadcastStateChanged, this));
    FOR_EACH_OBSERVER(OpSitePatcher::Observer, observers_, OnStateChanged());
  }
}

void OpSitePatcher::BroadcastStateChanged() {
  RenderProcessHost::iterator i(RenderProcessHost::AllHostsIterator());
  for (; !i.IsAtEnd(); i.Advance())
    SendEnable(i.GetCurrentValue());
}

void OpSitePatcher::SendEnable(content::RenderProcessHost* process) {
  process->Send(new OpMsg_SitePrefsEnable(config_.sitepatcher_enabled));
  process->Send(new OpMsg_BrowserJsEnable(config_.sitepatcher_enabled));
}

void OpSitePatcher::AddObserver(OpSitePatcher::Observer* observer) {
  observers_.AddObserver(observer);
}

void OpSitePatcher::RemoveObserver(OpSitePatcher::Observer* observer) {
  observers_.RemoveObserver(observer);
}
