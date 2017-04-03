// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#include "net/turbo/turbo_cache_synchronizer.h"

#include <algorithm>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/io_buffer.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/turbo/turbo_cache_entry.h"
#include "net/turbo/turbo_cache_enumerator.h"
#include "net/turbo/turbo_session_pool.h"

#if 1
# define TURBO_DEBUG(args) void(0)
#else
# define TURBO_DEBUG(args) printf args
#endif

namespace net {

namespace {

bool IsTurboEnabled() {
  return HttpStreamFactory::turbo2_enabled();
}

bool IsTurboSSLEnabled() {
  return false;
}

}  // namespace

TurboCacheSynchronizer::TurboCacheSynchronizer(
    base::WeakPtr<TurboSessionPool> turbo_session_pool)
    : cache_(NULL),
      turbo_session_pool_(turbo_session_pool),
      read_cache_list_entry_(NULL),
      write_cache_list_entry_(NULL),
      has_read_cache_list_(false),
      did_read_cache_list_(false),
      has_enumerated_cache_(false),
      has_sent_cache_list_(false),
      need_write_cache_list_(false),
      send_cache_list_scheduled_(false),
      read_cache_list_in_progress_(false),
      write_cache_list_in_progress_(false),
      scheduled_write_timer_(true, false),
      cache_list_update_timer_(false, false),
      weak_ptr_factory_(this) {
  scheduled_write_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(3),
      base::Bind(&TurboCacheSynchronizer::WriteCacheList,
                 weak_ptr_factory_.GetWeakPtr()));
}

void TurboCacheSynchronizer::SetHttpCache(HttpCache* cache) {
  if (cache_ != cache) {
    if (cache_)
      cache_->GetCurrentBackend()->RemoveObserver(this);

    cache_ = cache;
    cache_->GetCurrentBackend()->AddObserver(this);
  }

  if (turbo_session_pool_->HasProxySession())
    ReadCacheList();

  need_write_cache_list_ = true;
}

void TurboCacheSynchronizer::ClearHttpCache() {
  if (cache_)
    cache_->GetCurrentBackend()->RemoveObserver(this);
  cache_ = NULL;
}

void TurboCacheSynchronizer::TriggerSend() {
  if (has_read_cache_list_ && !has_sent_cache_list_)
    CacheListReady();
}

void TurboCacheSynchronizer::ResetSyncState() {
  has_enumerated_cache_ = has_sent_cache_list_ = false;
}

void TurboCacheSynchronizer::OnFirstTurboSession() {
  if (cache_ && !has_sent_cache_list_) {
    if (!has_read_cache_list_) {
      ReadCacheList();
    } else {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(&TurboCacheSynchronizer::CacheListReady,
                                weak_ptr_factory_.GetWeakPtr()));
    }
  }
}

void TurboCacheSynchronizer::OnTurboConnectionLost() {
  has_sent_cache_list_ = false;
  scheduled_write_timer_.Stop();
}

TurboCacheSynchronizer::~TurboCacheSynchronizer() {
  if (initial_enumerator_.get()) {
    initial_enumerator_->Stop();
    initial_enumerator_ = NULL;
  }

  if (read_cache_list_entry_)
    read_cache_list_entry_->Close();
  if (write_cache_list_entry_)
    write_cache_list_entry_->Close();
}

void TurboCacheSynchronizer::OnEntryDoomed(const std::string& key) {
  TurboCacheEntry entry(key);

  if (synchronized_entries_.count(entry) != 0) {
    MaybeStartCacheListTimer();

    doomed_entries_.insert(entry);
  }
}

void TurboCacheSynchronizer::OnCacheCleared() {
  doomed_entries_.insert(synchronized_entries_.begin(),
                         synchronized_entries_.end());

  if (!send_cache_list_scheduled_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&TurboCacheSynchronizer::SendCacheList,
                              weak_ptr_factory_.GetWeakPtr()));
    send_cache_list_scheduled_ = true;
  }

  cache_list_update_timer_.Stop();
}

void TurboCacheSynchronizer::ReadCacheList() {
  DCHECK(!write_cache_list_in_progress_ && !write_cache_list_entry_);
  DCHECK(!read_cache_list_in_progress_ && !read_cache_list_entry_);

  read_cache_list_in_progress_ = true;

  int result = cache_->GetCurrentBackend()->OpenEntry(
      "turbo:cache", &read_cache_list_entry_,
      base::Bind(&TurboCacheSynchronizer::OpenReadCacheListCompleted,
                 weak_ptr_factory_.GetWeakPtr()));

  if (result != ERR_IO_PENDING)
    OpenReadCacheListCompleted(result);
}

void TurboCacheSynchronizer::OpenReadCacheListCompleted(int result) {
  if (result == OK) {
    DCHECK(read_cache_list_entry_);
    TURBO_DEBUG(("CACHE LIST OPENED FOR READING\n"));
    DoReadCacheList();
  } else {
    read_cache_list_in_progress_ = false;
    DCHECK(!read_cache_list_entry_);
    TURBO_DEBUG(
        ("FAILED TO OPEN CACHE LIST FOR READING: %s\n", ErrorToString(result)));
    if (!has_read_cache_list_)
      CacheListReady();
  }
}

void TurboCacheSynchronizer::DoReadCacheList() {
  DCHECK(read_cache_list_in_progress_ && read_cache_list_entry_);

  int buf_len = read_cache_list_entry_->GetDataSize(0);

  if (buf_len != 0) {
    read_cache_list_buf_ = new IOBuffer(buf_len);

    int result = read_cache_list_entry_->ReadData(
        0, 0, read_cache_list_buf_.get(), buf_len,
        base::Bind(&TurboCacheSynchronizer::ReadCacheListCompleted,
                   weak_ptr_factory_.GetWeakPtr()));

    if (result != ERR_IO_PENDING)
      ReadCacheListCompleted(result);
  } else {
    read_cache_list_entry_->Close();
    read_cache_list_entry_ = NULL;
    read_cache_list_in_progress_ = false;
  }
}

void TurboCacheSynchronizer::ReadCacheListCompleted(int result) {
  read_cache_list_entry_->Close();
  read_cache_list_entry_ = NULL;
  read_cache_list_in_progress_ = false;

  if (result > 0) {
    DCHECK_EQ(0, result % 4);

    int* buf_data = reinterpret_cast<int*>(read_cache_list_buf_->data());
    int count = result / 4;

    TURBO_DEBUG(("LOADED CACHE LIST:\n"));

    for (int index = 0; index < count; ++index) {
      TURBO_DEBUG(("  %08x\n", *buf_data));
      synchronized_entries_.insert(TurboCacheEntry(*buf_data++));
    }

    read_cache_list_buf_ = NULL;
    did_read_cache_list_ = true;
    need_write_cache_list_ = false;

    CacheListReady();
  } else {
    TURBO_DEBUG(("FAILED TO READ CACHE LIST: %s\n", ErrorToString(result)));

    CacheListReady();
  }
}

void TurboCacheSynchronizer::CacheListReady() {
  has_read_cache_list_ = true;

  if (!has_enumerated_cache_) {
    if (!initial_enumerator_.get()) {
      initial_enumerator_ = new TurboCacheEnumerator(IsTurboSSLEnabled());

      int result = initial_enumerator_->Start(
          cache_, base::Bind(&TurboCacheSynchronizer::CacheEnumerationCompleted,
                             weak_ptr_factory_.GetWeakPtr()));

      if (result != ERR_IO_PENDING)
        CacheEnumerationCompleted(result);
    }

    return;
  }

  if (!has_sent_cache_list_ && IsTurboEnabled())
    SendCacheList();
}

void TurboCacheSynchronizer::WriteCacheList() {
  DCHECK(!read_cache_list_in_progress_);

  if (!cache_ || !need_write_cache_list_ || write_cache_list_in_progress_)
    return;

  write_cache_list_in_progress_ = true;

  int result = cache_->GetCurrentBackend()->OpenEntry(
      "turbo:cache", &write_cache_list_entry_,
      base::Bind(&TurboCacheSynchronizer::OpenWriteCacheListCompleted,
                 weak_ptr_factory_.GetWeakPtr()));

  if (result != ERR_IO_PENDING)
    OpenWriteCacheListCompleted(result);
}

void TurboCacheSynchronizer::OpenWriteCacheListCompleted(int result) {
  if (result == OK) {
    TURBO_DEBUG(("CACHE LIST OPENED FOR WRITING\n"));
    DoWriteCacheList();
  } else {
    DCHECK(!write_cache_list_entry_);
    TURBO_DEBUG(("FAILED TO OPEN CACHE LIST FOR WRITING: %s\n",
                 ErrorToString(result)));

    int result = cache_->GetCurrentBackend()->CreateEntry(
        "turbo:cache", &write_cache_list_entry_,
        base::Bind(&TurboCacheSynchronizer::CreateCacheListCompleted,
                   weak_ptr_factory_.GetWeakPtr()));

    if (result != ERR_IO_PENDING)
      CreateCacheListCompleted(result);
  }
}

void TurboCacheSynchronizer::CreateCacheListCompleted(int result) {
  if (result == OK) {
    TURBO_DEBUG(("CACHE LIST CREATED\n"));
    DoWriteCacheList();
  } else {
    DCHECK(!write_cache_list_entry_);
    TURBO_DEBUG(("FAILED TO CREATE CACHE LIST: %s\n", ErrorToString(result)));
    write_cache_list_in_progress_ = false;
  }
}

void TurboCacheSynchronizer::DoWriteCacheList() {
  DCHECK(write_cache_list_in_progress_ && write_cache_list_entry_);

  int buf_len = synchronized_entries_.size() * 4;

  write_cache_list_buf_ = new IOBuffer(buf_len);

  int* buf_data = reinterpret_cast<int*>(write_cache_list_buf_->data());

  std::set<TurboCacheEntry>::const_iterator iter =
      synchronized_entries_.begin();

  TURBO_DEBUG(("STORED CACHE LIST:\n"));

  while (iter != synchronized_entries_.end()) {
    TURBO_DEBUG(("  %08x\n", iter->url_crc32()));
    *buf_data++ = iter++->url_crc32();
  }

  need_write_cache_list_ = false;

  int result = write_cache_list_entry_->WriteData(
      0, 0, write_cache_list_buf_.get(), buf_len,
      base::Bind(&TurboCacheSynchronizer::WriteCacheListCompleted,
                 weak_ptr_factory_.GetWeakPtr()),
      true);

  if (result != ERR_IO_PENDING)
    WriteCacheListCompleted(result);
}

void TurboCacheSynchronizer::WriteCacheListCompleted(int result) {
  if (result < 0)
    TURBO_DEBUG(("FAILED TO WRITE CACHE LIST: %s\n", ErrorToString(result)));
  else
    TURBO_DEBUG(("WROTE CACHE LIST: %d bytes\n", result));

  write_cache_list_entry_->Close();
  write_cache_list_entry_ = NULL;
  write_cache_list_buf_ = NULL;
  write_cache_list_in_progress_ = false;
}

void TurboCacheSynchronizer::AddSynchronizedURL(const GURL& gurl) {
  TurboCacheEntry entry(gurl.spec());
  if (synchronized_entries_.insert(entry).second) {
    TURBO_DEBUG(("ADDED SYNCHRONIZED URL: %s\n", gurl.spec().c_str()));
    need_write_cache_list_ = true;
    scheduled_write_timer_.Reset();
  }
}

void TurboCacheSynchronizer::CacheEnumerationCompleted(int result) {
  if (result == OK && turbo_session_pool_->HasProxySession()) {
    has_enumerated_cache_ = true;

    if (!has_sent_cache_list_ && IsTurboEnabled())
      SendCacheList();
  }

  initial_enumerator_ = NULL;
}

void TurboCacheSynchronizer::MaybeStartCacheListTimer() {
  if (doomed_entries_.size() == 0 && !cache_list_update_timer_.IsRunning()) {
    cache_list_update_timer_.Start(
        FROM_HERE, base::TimeDelta::FromSeconds(1),
        base::Bind(&TurboCacheSynchronizer::SendCacheList,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void TurboCacheSynchronizer::SendCacheList() {
  if (!turbo_session_pool_->HasProxySession())
    return;

  SpdySession* proxy_session = turbo_session_pool_->GetProxySession();

  has_sent_cache_list_ = true;
  send_cache_list_scheduled_ = false;

  std::vector<TurboCacheEntry> added_entries;
  std::vector<TurboCacheEntry> removed_entries;
  std::set<TurboCacheEntry>::const_iterator iter;

  if (initial_enumerator_.get()) {
    TURBO_DEBUG(("ANALYZING CACHE LIST:\n"));

    for (iter = initial_enumerator_->entries().begin();
         iter != initial_enumerator_->entries().end();
         ++iter) {
      if (synchronized_entries_.count(*iter) == 0) {
        TURBO_DEBUG(("+ %08x\n", iter->url_crc32()));
        added_entries.push_back(*iter);
      }
    }

    for (iter = synchronized_entries_.begin();
         iter != synchronized_entries_.end();
         ++iter) {
      if (initial_enumerator_->entries().count(*iter) == 0) {
        TURBO_DEBUG(("- %08x\n", iter->url_crc32()));
        removed_entries.push_back(*iter);
      }
    }

    size_t delta_size = added_entries.size() + removed_entries.size();

    if (delta_size == 0) {
      // No changes.
      TURBO_DEBUG(("  NO CHANGES\n"));

      // If we (successfully) read a stored cache list, we don't need to tell
      // the Turbo proxy anything.  If we didn't read a stored cache list, we
      // might have restarted with a cleared cache, in which case we should send
      // the (empty) cache list to the Turbo proxy.
      if (did_read_cache_list_)
        return;
    }

    if (delta_size >= initial_enumerator_->entries().size() ||
        !did_read_cache_list_) {
      // A "full" cache list would be more compact than an "incremental" one,
      // *or* we don't know what the proxy thinks is in our cache and thus need
      // to tell the proxy to forget what it (might) know and then add
      // everything we do have in our cache.
      std::vector<TurboCacheEntry> entries(
          initial_enumerator_->entries().begin(),
          initial_enumerator_->entries().end());
      TURBO_DEBUG(("SENDING FULL CACHE LIST: +%lu entries\n", entries.size()));
      proxy_session->SendFullCacheList(entries);
    } else if (delta_size > 0) {
      TURBO_DEBUG(("SENDING INCREMENTAL CACHE LIST: -%lu/+%lu entries\n",
                   removed_entries.size(), added_entries.size()));
      proxy_session->SendIncrementalCacheList(added_entries, removed_entries);
    }

    synchronized_entries_.clear();
    synchronized_entries_.swap(initial_enumerator_->entries());
  } else if (!doomed_entries_.empty()) {
    for (iter = doomed_entries_.begin(); iter != doomed_entries_.end(); ++iter)
      synchronized_entries_.erase(*iter);

    if (synchronized_entries_.size() < doomed_entries_.size()) {
      std::vector<TurboCacheEntry> entries(synchronized_entries_.begin(),
                                           synchronized_entries_.end());
      proxy_session->SendFullCacheList(entries);
    } else {
      std::vector<TurboCacheEntry> added_entries;
      std::vector<TurboCacheEntry> removed_entries(doomed_entries_.begin(),
                                                   doomed_entries_.end());
      proxy_session->SendIncrementalCacheList(added_entries, removed_entries);
    }

    doomed_entries_.clear();
  }

  WriteCacheList();
}

}  // namespace net
