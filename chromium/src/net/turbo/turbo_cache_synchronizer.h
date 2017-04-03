// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#ifndef NET_TURBO_TURBO_CACHE_SYNCHRONIZER_H_
#define NET_TURBO_TURBO_CACHE_SYNCHRONIZER_H_

#include <set>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "net/disk_cache/disk_cache.h"

class GURL;

namespace net {

class HttpCache;
class TurboCacheEntry;
class TurboCacheEnumerator;
class TurboSessionPool;

// Overview of the cache synchronization algorithm:
//
// |synchronized_entries_| always holds a list of entries that the proxy should
// think we have in our cache. This list is written into the disk cache under
// the "turbo:cache" key after each change to |synchronized_entries_|. On
// startup, this cache entry is read back into |synchronized_entries_|.
//
// Each time a URL is downloaded, the proxy assumes we have it in our cache,
// thus we add it to |synchronized_entries_|.
//
// Before sending any cache list changes to the proxy, the current state of the
// cache is enumerated using |initial_enumerator_|. After |initial_enumerator_|
// is populated, its contents are compared to |synchronized_entries_| and any
// differences are sent to the proxy.
class NET_EXPORT TurboCacheSynchronizer
    : public disk_cache::Backend::Observer {
 public:
  explicit TurboCacheSynchronizer(
      base::WeakPtr<TurboSessionPool> turbo_session_pool);
  ~TurboCacheSynchronizer() override;

  void SetHttpCache(HttpCache* cache);
  void ClearHttpCache();

  void AddSynchronizedURL(const GURL& gurl);

  // TODO(michalp): Rewrite the following methods as an observer of
  // TurboSessionPool.

  // If local cache state is dirty, trigger sync.
  void TriggerSend();
  // Mark local cache state as dirty to force re-enumeration and sending an
  // update at first opportunity.
  void ResetSyncState();
  // Called when the first Turbo 2 session becomes available to enumerate and
  // send current cache state.
  void OnFirstTurboSession();
  // Called when the last Turbo 2 session is closed.
  void OnTurboConnectionLost();

 private:
  // disk_cache::Backend::Observer implementation
  void OnEntryDoomed(const std::string& key) override;
  void OnCacheCleared() override;

  // Cache list cache entry reading.
  void ReadCacheList();
  void OpenReadCacheListCompleted(int result);
  void DoReadCacheList();
  void ReadCacheListCompleted(int result);
  void CacheListReady();

  // Cache list cache entry writing.
  void WriteCacheList();
  void OpenWriteCacheListCompleted(int result);
  void CreateCacheListCompleted(int result);
  void DoWriteCacheList();
  void WriteCacheListCompleted(int result);

  void CacheEnumerationCompleted(int result);

  void MaybeStartCacheListTimer();

  void SendCacheList();

  HttpCache* cache_;
  base::WeakPtr<TurboSessionPool> turbo_session_pool_;

  disk_cache::Entry* read_cache_list_entry_;
  disk_cache::Entry* write_cache_list_entry_;

  scoped_refptr<IOBuffer> read_cache_list_buf_;
  scoped_refptr<IOBuffer> write_cache_list_buf_;

  // Set of cache entries that (we believe) the Turbo server thinks we have in
  // our cache.
  std::set<TurboCacheEntry> synchronized_entries_;

  // Set of cache entries that are included in 'synchronized_entries_' and have
  // been doomed.  IOW, cache entries the Turbo server thinks we have in our
  // cache but that we know we don't.
  std::set<TurboCacheEntry> doomed_entries_;

  scoped_refptr<TurboCacheEnumerator> initial_enumerator_;

  // TODO(michalp): Rename the following two flags to something more meaningful.

  // Has CacheListReady been called at least once already? This should indicate
  // that we have finished reading "turbo:cache" from the cache, possibly with a
  // failure.
  bool has_read_cache_list_;
  // Was there a successful attempt to read "turbo:cache"?
  bool did_read_cache_list_;
  bool has_enumerated_cache_;
  bool has_sent_cache_list_;
  bool need_write_cache_list_;
  bool send_cache_list_scheduled_;

  bool read_cache_list_in_progress_;
  bool write_cache_list_in_progress_;

  // Timer that tells us to store an updated cache list entry.
  base::Timer scheduled_write_timer_;

  // Timer that tells us to send an updated cache list to the Turbo server.
  base::Timer cache_list_update_timer_;

  base::WeakPtrFactory<TurboCacheSynchronizer> weak_ptr_factory_;
};

}  // namespace net

#endif  // NET_TURBO_TURBO_CACHE_SYNCHRONIZER_H_
