// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#ifndef NET_TURBO_TURBO_CACHE_ENUMERATOR_H_
#define NET_TURBO_TURBO_CACHE_ENUMERATOR_H_

#include <memory>
#include <set>

#include "net/base/completion_callback.h"
#include "net/disk_cache/disk_cache.h"

namespace net {

class HttpCache;
class TurboCacheEntry;

class TurboCacheEnumerator : public base::RefCounted<TurboCacheEnumerator> {
 public:
  // Create an enumerator, |with_https| may be true if the Turbo connection is
  // encrypted and we have enabled https over Turbo, then https urls will be
  // included in the cache synchronization too.
  explicit TurboCacheEnumerator(bool with_https);

  // Start the enumeration.  If this function returns ERR_IO_PENDING,
  // the callback is called when the enumeration has finished.  If
  // this function returns OK, the enumeration finished synchronously.
  int Start(HttpCache* cache, const CompletionCallback& callback);

  // Stop the enumeration, if it's still in progress.  The callback given
  // to Start() will not be called.
  void Stop();

  std::set<TurboCacheEntry>& entries() { return entries_; }

 private:
  friend class base::RefCounted<TurboCacheEnumerator>;
  ~TurboCacheEnumerator();

  void OpenNextEntryCompleted(int result);

  int Process(int result);

  std::set<TurboCacheEntry> entries_;
  CompletionCallback callback_;

  HttpCache* cache_;
  disk_cache::Backend* backend_;
  std::unique_ptr<disk_cache::Backend::Iterator> iter_;
  disk_cache::Entry* entry_;
  bool with_https_;
};

}  // namespace net

#endif  // NET_TURBO_TURBO_CACHE_ENUMERATOR_H_
