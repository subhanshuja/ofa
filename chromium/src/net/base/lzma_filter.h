// Copyright (c) 2013 Opera Software ASA. All rights reserved.

// LzmaFilter is a subclass of Filter. See the latter's header file filter.h
// for sample usage.

#ifndef NET_BASE_LZMA_FILTER_H_
#define NET_BASE_LZMA_FILTER_H_

#include "net/filter/filter.h"
#include "third_party/lzma_sdk/LzmaDec.h"

namespace net {

class LzmaFilter : public Filter {
 public:
  ~LzmaFilter() override;

  // Decode the pre-filter data and writes the output into |dest_buffer|
  // The function returns FilterStatus. See filter.h for its description.
  //
  // Upon entry, *dest_len is the total size (in number of chars) of the
  // destination buffer. Upon exit, *dest_len is the actual number of chars
  // written into the destination buffer.
  FilterStatus ReadFilteredData(char* dest_buffer, int* dest_len) override;

 private:
  // Only to be instantiated by Filter::Factory.
  LzmaFilter();
  friend class Filter;

  // Take as many bytes from |next_stream_data_| as possible and copy them into
  // |header_buffer_|. May be called multiple times
  void FillHeaderBufferFromStream();
  bool IsHeaderBufferFilled() const;
  void ResetHeaderBuffer();

  // Lzma Header is the Lzma properties followed by the
  // uncompressed length, 8 bytes, (which we don't look at)
  static const size_t kLzmaHeaderSize = LZMA_PROPS_SIZE + 8;

  bool allocated_;
  CLzmaDec state_;
  ISzAlloc alloc_;

  // The LZMA header needs to be received completely before we can initialize
  // decoding. Since the network can be slow and send it in small packets, we
  // have this buffer to cache it.
  char header_buffer_[kLzmaHeaderSize];
  size_t header_buffer_current_size_;

  DISALLOW_COPY_AND_ASSIGN(LzmaFilter);
};

}  // namespace net

#endif  // NET_BASE_LZMA_FILTER_H__
