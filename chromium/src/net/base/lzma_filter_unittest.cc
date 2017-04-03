// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "net/base/lzma_filter.h"

#include <memory>
#include <vector>

#include "base/memory/ptr_util.h"
#include "net/base/io_buffer.h"
#include "net/filter/mock_filter_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {
namespace {
const char kUncompressedContent[] = "uncompressed content";
const size_t kUncompressedContentSize = arraysize(kUncompressedContent);

// This is the precomputed result of LZMA-compressing |kUncompressedContent|.
// Used Python's lzma library:
// import lzma
// lzc = lzma.LZMACompressor(format=lzma.FORMAT_ALONE)
// result = lzc.compress(b"uncompressed content") + lzc.flush()
const unsigned char kCompressedContent[] = {
    0x5d, 0x00, 0x00, 0x80, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x00, 0x3a, 0x9b, 0x88, 0x8e, 0x26, 0x65, 0xa7, 0x66,
    0xc3, 0xb8, 0x3b, 0xea, 0xfe, 0xd4, 0x9c, 0x35, 0x5c, 0xaa, 0x5e,
    0xc4, 0x47, 0xfd, 0xd8, 0x7b, 0xff, 0xee, 0x56, 0x20, 0x00};
const size_t kCompressedContentSize = arraysize(kCompressedContent);

const unsigned char kInvalidCompressedContent[] = {
    0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x0, 0x0, 0x1, 0x69, 0x22, 0xde, 0x36,
    0x2,  0x0,  0x21, 0x1,  0x16, 0x0, 0x0, 0x0, 0x74, 0x2f, 0xe5, 0xa3};
const size_t kInvalidCompressedContentSize = arraysize(kCompressedContent);

}  // namespace

class LzmaFilterTest : public testing::Test {
 public:
  std::unique_ptr<LzmaFilter> create_filter() const {
    std::vector<Filter::FilterType> filter_type;
    filter_type.push_back(Filter::FILTER_TYPE_LZMA);
    MockFilterContext filter_context;
    return base::WrapUnique(static_cast<LzmaFilter*>(
        Filter::Factory(filter_type, filter_context).release()));
  }
};

TEST_F(LzmaFilterTest, DecompressCompleteContent) {
  std::unique_ptr<LzmaFilter> filter = create_filter();
  ASSERT_GT(filter->stream_buffer_size(),
            static_cast<int>(kCompressedContentSize));
  memcpy(filter->stream_buffer()->data(), kCompressedContent,
         kCompressedContentSize);
  filter->FlushStreamBuffer(kCompressedContentSize);
  char uncompressed_data[kUncompressedContentSize];
  // In-out param:
  int uncompressed_data_size = kUncompressedContentSize;
  EXPECT_EQ(Filter::FILTER_DONE,
            filter->ReadData(uncompressed_data, &uncompressed_data_size));
  // Uncompressed data doesn't contain the trailing \0
  ASSERT_EQ(static_cast<int>(kUncompressedContentSize) - 1,
            uncompressed_data_size);
  EXPECT_EQ(0, memcmp(uncompressed_data, kUncompressedContent,
                      uncompressed_data_size));

  // The data should be consumed from the stream.
  EXPECT_EQ(0, filter->stream_data_len());
}

TEST_F(LzmaFilterTest, DecompressIncompleteContent) {
  std::unique_ptr<LzmaFilter> filter = create_filter();
  ASSERT_GT(filter->stream_buffer_size(),
            static_cast<int>(kCompressedContentSize));
  // Instead of copying the entire data, leave out some elements for later.
  const size_t remainder_size = 15;
  const size_t reduced_size = kCompressedContentSize - remainder_size;
  memcpy(filter->stream_buffer()->data(), kCompressedContent, reduced_size);
  filter->FlushStreamBuffer(reduced_size);
  char uncompressed_data[kUncompressedContentSize];
  int partial_uncompressed_data_size = kUncompressedContentSize;
  // Filter returns FILTER_NEED_MORE_DATA, as there should be more data to
  // decompress the entire string.
  EXPECT_EQ(
      Filter::FILTER_NEED_MORE_DATA,
      filter->ReadData(uncompressed_data, &partial_uncompressed_data_size));

  // The result size is smaller than the complete uncompressed content size.
  EXPECT_LT(partial_uncompressed_data_size,
            static_cast<int>(kUncompressedContentSize));

  // The data should be consumed from the stream.
  EXPECT_EQ(0, filter->stream_data_len());

  // Now send the reminder of the data.
  memcpy(filter->stream_buffer()->data(), kCompressedContent + reduced_size,
         remainder_size);
  filter->FlushStreamBuffer(remainder_size);

  // Read into the rest of the output buffer, so offset the output by
  // |reported_partial_uncompressed_data_size|.
  int uncompressed_remainder_size = static_cast<int>(kUncompressedContentSize) -
                                    partial_uncompressed_data_size;
  EXPECT_EQ(Filter::FILTER_DONE,
            filter->ReadData(uncompressed_data + partial_uncompressed_data_size,
                             &uncompressed_remainder_size));

  const int uncompressed_data_size =
      partial_uncompressed_data_size + uncompressed_remainder_size;
  // Uncompressed data doesn't contain the trailing \0
  ASSERT_EQ(static_cast<int>(kUncompressedContentSize) - 1,
            uncompressed_data_size);
  EXPECT_EQ(0, memcmp(uncompressed_data, kUncompressedContent,
                      uncompressed_data_size));

  // The data should be consumed from the stream.
  EXPECT_EQ(0, filter->stream_data_len());
}

TEST_F(LzmaFilterTest, DecompressIncompleteHeader) {
  // Like DecompressIncompleteContent but make sure we make the split within
  // the LZMA header.
  std::unique_ptr<LzmaFilter> filter = create_filter();
  ASSERT_GT(filter->stream_buffer_size(),
            static_cast<int>(kCompressedContentSize));
  // Send less data than required to decode the LZMA header:
  const size_t reduced_size = LZMA_PROPS_SIZE - 2;
  const size_t remainder_size = kCompressedContentSize - reduced_size;
  memcpy(filter->stream_buffer()->data(), kCompressedContent, reduced_size);
  filter->FlushStreamBuffer(reduced_size);
  char uncompressed_data[kUncompressedContentSize];
  int partial_uncompressed_data_size = kUncompressedContentSize;
  // Filter returns FILTER_NEED_MORE_DATA, as there should be more data to
  // decompress the entire string.
  EXPECT_EQ(
      Filter::FILTER_NEED_MORE_DATA,
      filter->ReadData(uncompressed_data, &partial_uncompressed_data_size));

  // No data should be decoded yet
  EXPECT_EQ(0, partial_uncompressed_data_size);

  // The data should be consumed from the stream.
  EXPECT_EQ(0, filter->stream_data_len());

  // Now send the reminder of the data.
  memcpy(filter->stream_buffer()->data(), kCompressedContent + reduced_size,
         remainder_size);
  filter->FlushStreamBuffer(remainder_size);

  // Read into the rest of the output buffer, so offset the output by
  // |reported_partial_uncompressed_data_size|.
  int uncompressed_remainder_size = static_cast<int>(kUncompressedContentSize) -
                                    partial_uncompressed_data_size;
  EXPECT_EQ(Filter::FILTER_DONE,
            filter->ReadData(uncompressed_data + partial_uncompressed_data_size,
                             &uncompressed_remainder_size));

  const int uncompressed_data_size =
      partial_uncompressed_data_size + uncompressed_remainder_size;
  // Uncompressed data doesn't contain the trailing \0
  ASSERT_EQ(static_cast<int>(kUncompressedContentSize) - 1,
            uncompressed_data_size);
  EXPECT_EQ(0, memcmp(uncompressed_data, kUncompressedContent,
                      uncompressed_data_size));

  // The data should be consumed from the stream.
  EXPECT_EQ(0, filter->stream_data_len());
}

TEST_F(LzmaFilterTest, FailWithInvalidContent) {
  std::unique_ptr<LzmaFilter> filter = create_filter();
  ASSERT_GT(filter->stream_buffer_size(),
            static_cast<int>(kInvalidCompressedContentSize));
  memcpy(filter->stream_buffer()->data(), kInvalidCompressedContent,
         kInvalidCompressedContentSize);
  filter->FlushStreamBuffer(kInvalidCompressedContentSize);
  char uncompressed_data[kUncompressedContentSize];
  // In-out param:
  int uncompressed_data_size = kUncompressedContentSize;
  EXPECT_EQ(Filter::FILTER_ERROR,
            filter->ReadData(uncompressed_data, &uncompressed_data_size));
}

}  // namespace net
