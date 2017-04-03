// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#include "net/base/lzma_filter.h"

#include <algorithm>

namespace net {

static void* AllocForLzma(void* p, size_t size) {
  return operator new(size);
}

static void FreeForLzma(void* p, void* address) {
  if (address)
    operator delete(address);
}

LzmaFilter::LzmaFilter()
    : Filter(FILTER_TYPE_LZMA),
      allocated_(false),
      header_buffer_current_size_(0) {
  alloc_.Alloc = AllocForLzma;
  alloc_.Free = FreeForLzma;
}

LzmaFilter::~LzmaFilter() {
  if (allocated_)
    LzmaDec_Free(&state_, &alloc_);
}

void LzmaFilter::FillHeaderBufferFromStream() {
  static_assert(sizeof(header_buffer_) == kLzmaHeaderSize,
                "header_buffer_ must be able to contain an lzma header");
  // Copy kLzmaHeaderSize bytes of data from the stream to |header_buffer_|.
  // Since the size of the stream may be smaller than kLzmaHeaderSize, be
  // prepared to keep copying as much as possible in subsequent calls to this
  // function until we have filled |header_buffer_|.
  const size_t free_space_in_buffer =
      kLzmaHeaderSize - header_buffer_current_size_;
  const size_t bytes_to_copy =
      std::min(static_cast<size_t>(stream_data_len_), free_space_in_buffer);
  std::copy(next_stream_data_, next_stream_data_ + bytes_to_copy,
            header_buffer_ + header_buffer_current_size_);
  header_buffer_current_size_ += bytes_to_copy;

  // Consume the copied data from the stream.
  next_stream_data_ += bytes_to_copy;
  stream_data_len_ -= bytes_to_copy;
}

bool LzmaFilter::IsHeaderBufferFilled() const {
  return header_buffer_current_size_ == kLzmaHeaderSize;
}

void LzmaFilter::ResetHeaderBuffer() {
  header_buffer_current_size_ = 0;
}

Filter::FilterStatus LzmaFilter::ReadFilteredData(char* dest_buffer,
                                                  int* dest_len) {
  if (!next_stream_data_ || stream_data_len_ <= 0) {
    ResetHeaderBuffer();
    return Filter::FILTER_ERROR;
  }

  if (!allocated_) {
    FillHeaderBufferFromStream();
    if (!IsHeaderBufferFilled()) {
      // This is a precaution, the caller shouldn't read the output because we
      // won't return a success.
      *dest_len = 0;
      // Wait for more data. This function will be called again when new data is
      // in the stream.
      return Filter::FILTER_NEED_MORE_DATA;
    }
    // |header_buffer_| should now contain the entire lzma header, initialize
    // the decoder:

    LzmaDec_Construct(&state_);

    SRes res = LzmaDec_Allocate(&state_,
                                reinterpret_cast<Byte*>(header_buffer_),
                                LZMA_PROPS_SIZE,
                                &alloc_);
    if (res != SZ_OK) {
      ResetHeaderBuffer();
      return Filter::FILTER_ERROR;
    }

    LzmaDec_Init(&state_);
    allocated_ = true;
  }

  SizeT destLen = *dest_len;
  SizeT srcLen = stream_data_len_;
  ELzmaStatus status;
  SRes res = LzmaDec_DecodeToBuf(&state_,
                                 reinterpret_cast<Byte*>(dest_buffer),
                                 &destLen,
                                 reinterpret_cast<Byte*>(next_stream_data_),
                                 &srcLen,
                                 LZMA_FINISH_ANY,
                                 &status);

  next_stream_data_ += srcLen;
  stream_data_len_ -= srcLen;
  *dest_len = destLen;

  if (res == SZ_OK) {
    if (status == LZMA_STATUS_NEEDS_MORE_INPUT)
      return Filter::FILTER_NEED_MORE_DATA;
    else if (status == LZMA_STATUS_NOT_FINISHED ||
             status == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK)
      return Filter::FILTER_OK;
    else if (status == LZMA_STATUS_FINISHED_WITH_MARK)
      return Filter::FILTER_DONE;
  }

  return Filter::FILTER_ERROR;
}

}  // namespace net
