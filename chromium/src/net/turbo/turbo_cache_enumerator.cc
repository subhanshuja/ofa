// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#include "net/turbo/turbo_cache_enumerator.h"

#include <string>

#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/turbo/turbo_cache_entry.h"

#if 1
# define TURBO_DEBUG(args) void(0)
#else
# define TURBO_DEBUG(args) printf args
#endif

namespace net {

TurboCacheEnumerator::TurboCacheEnumerator(bool with_https)
    : cache_(NULL), backend_(NULL), entry_(NULL), with_https_(with_https) {
}

TurboCacheEnumerator::~TurboCacheEnumerator() {
  if (entry_)
    entry_->Close();
}

int TurboCacheEnumerator::Start(HttpCache* cache,
                                const CompletionCallback& callback) {
  cache_ = cache;
  backend_ = cache->GetCurrentBackend();
  callback_ = callback;

  if (!iter_)
    iter_ = backend_->CreateIterator();
  int result = iter_->OpenNextEntry(
      &entry_,
      base::Bind(&TurboCacheEnumerator::OpenNextEntryCompleted, this));

  result = Process(result);

  // Finished synchronously (perhaps empty cache).
  if (result == ERR_FAILED)
    result = OK;

  if (result == OK)
    TURBO_DEBUG(("ENUMERATE: FINISHED\n"));
  else if (result != ERR_IO_PENDING)
    TURBO_DEBUG(("ENUMERATE: FAILED\n"));

  return result;
}

void TurboCacheEnumerator::Stop() {
  iter_.reset();
}

void TurboCacheEnumerator::OpenNextEntryCompleted(int result) {
  result = Process(result);

  DCHECK_NE(OK, result);

  switch (result) {
    case ERR_IO_PENDING:
      return;

    case ERR_FAILED:
      // No more entries.
      TURBO_DEBUG(("ENUMERATE: FINISHED (ASYNCHRONOUSLY)\n"));
      callback_.Run(OK);
      break;

    default:
      TURBO_DEBUG(("ENUMERATE: FAILED (ASYNCHRONOUSLY)\n"));
      callback_.Run(result);
      break;
  }

  // Finished or failure; either way, we're done.
  iter_.reset();
}

int TurboCacheEnumerator::Process(int result) {
  while (result == OK) {
    if (entry_) {
      const std::string& url = entry_->GetKey();

      if (url.compare(0, 5, "http:") == 0 ||
          (with_https_ && url.compare(0, 6, "https:") == 0)) {
        TURBO_DEBUG(("ENUMERATE: ENTRY: %s\n", url.c_str()));
        entries_.insert(TurboCacheEntry(url));
      } else {
        TURBO_DEBUG(("ENUMERATE: IGNORING ENTRY: %s\n", url.c_str()));
      }

      entry_->Close();
      entry_ = NULL;
    }

    if (!iter_)
      iter_ = backend_->CreateIterator();
    result = iter_->OpenNextEntry(
        &entry_,
        base::Bind(&TurboCacheEnumerator::OpenNextEntryCompleted, this));
  }

  return result;
}

}  // namespace net
