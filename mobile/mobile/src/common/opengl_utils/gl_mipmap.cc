// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/opengl_utils/gl_mipmap.h"

#include <algorithm>
#include <cstdio>
#include <utility>

#include "android/bitmap.h"
#include "base/logging.h"
#include "base/memory/aligned_memory.h"
#include "cc/raster/texture_compressor.h"
#include "common/opengl_utils/gl_library.h"
#include "third_party/lz4/lib/lz4.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/src/core/SkMathPriv.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/codec/jpeg_codec.h"

namespace opera {

namespace {

size_t CalculateSize(GLMipMap::Format format, size_t width, size_t height) {
  switch (format) {
    case GLMipMap::kRGB:
    case GLMipMap::kRGBA:
      // RGB is actually RGBX.
      return width * height * 4;
    case GLMipMap::kATC:
    case GLMipMap::kDXT1:
    case GLMipMap::kETC1:
      // Opaque with block size = 8.
      return ((width + 3) / 4) * ((height + 3) / 4) * 8;
    case GLMipMap::kATCIA:
    case GLMipMap::kDXT5:
      // Transluscent with block size = 16.
      return ((width + 3) / 4) * ((height + 3) / 4) * 16;
      break;
    default:
      LOG(FATAL) << "Bad format when requesting mipmap size.";
      return 0;
  }
}

// Blend between two colors with equal weights.
uint32_t Mix2(uint32_t p0, uint32_t p1) {
  uint32_t r = (((p0 >> 0) & 0xff) + ((p1 >> 0) & 0xff)) / 2;
  uint32_t g = (((p0 >> 8) & 0xff) + ((p1 >> 8) & 0xff)) / 2;
  uint32_t b = (((p0 >> 16) & 0xff) + ((p1 >> 16) & 0xff)) / 2;
  uint32_t a = (((p0 >> 24) & 0xff) + ((p1 >> 24) & 0xff)) / 2;

  return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

// Blend between four colors with equal weights.
uint32_t Mix4(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3) {
  uint32_t r = (((p0 >> 0) & 0xff) + ((p1 >> 0) & 0xff) + ((p2 >> 0) & 0xff) +
                ((p3 >> 0) & 0xff)) /
               4;
  uint32_t g = (((p0 >> 8) & 0xff) + ((p1 >> 8) & 0xff) + ((p2 >> 8) & 0xff) +
                ((p3 >> 8) & 0xff)) /
               4;
  uint32_t b = (((p0 >> 16) & 0xff) + ((p1 >> 16) & 0xff) +
                ((p2 >> 16) & 0xff) + ((p3 >> 16) & 0xff)) /
               4;
  uint32_t a = (((p0 >> 24) & 0xff) + ((p1 >> 24) & 0xff) +
                ((p2 >> 24) & 0xff) + ((p3 >> 24) & 0xff)) /
               4;

  return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

// Anisotropic blending of colors.
void MipNonUniform(void* dst, const void* src, size_t length) {
  const uint32_t* s = reinterpret_cast<const uint32_t*>(src);
  uint32_t* d = reinterpret_cast<uint32_t*>(dst);
  for (size_t y = 0; y < length; ++y) {
    *d++ = Mix2(s[0], s[1]);
    s += 2;
  }
}

GLMipMap::MipVector CreateMips(size_t width,
                               size_t height,
                               const void* src,
                               GLMipMap::Format format) {
  GLMipMap::MipVector result;

  // Create an encoder if the format is compressed.
  std::unique_ptr<cc::TextureCompressor> encoder;
  switch (format) {
    case GLMipMap::kATC:
      encoder =
          cc::TextureCompressor::Create(cc::TextureCompressor::kFormatATC);
      break;
    case GLMipMap::kATCIA:
      encoder =
          cc::TextureCompressor::Create(cc::TextureCompressor::kFormatATCIA);
      break;
    case GLMipMap::kDXT1:
      encoder =
          cc::TextureCompressor::Create(cc::TextureCompressor::kFormatDXT1);
      break;
    case GLMipMap::kDXT5:
      encoder =
          cc::TextureCompressor::Create(cc::TextureCompressor::kFormatDXT5);
      break;
    case GLMipMap::kETC1:
      encoder =
          cc::TextureCompressor::Create(cc::TextureCompressor::kFormatETC1);
      break;
    default:
      break;
  }

  // Create base level.
  size_t num_src_bytes = CalculateSize(GLMipMap::kRGBA, width, height);
  GLMipMap::MipPointer mip(base::AlignedAlloc(num_src_bytes, 16),
                           base::AlignedFreeDeleter());
  memcpy(mip.get(), src, num_src_bytes);

  {
    if (encoder) {
      size_t num_bytes = CalculateSize(format, width, height);
      GLMipMap::MipPointer compressed_mip(base::AlignedAlloc(num_bytes, 16),
                                          base::AlignedFreeDeleter());
      encoder->Compress(reinterpret_cast<const uint8_t*>(mip.get()),
                        reinterpret_cast<uint8_t*>(compressed_mip.get()), width,
                        height, cc::TextureCompressor::kQualityHigh);
      result.push_back(compressed_mip);
    } else {
      result.push_back(mip);
    }
  }

  // Need to keep the 'parent_mip' level to generate each new mip level.
  GLMipMap::MipPointer parent_mip = mip;

  // Create higher levels.
  size_t parent_width = width;
  size_t parent_height = height;
  while (parent_width > 1 || parent_height > 1) {
    // Reduce the dimensions.
    size_t mip_width = std::max(parent_width >> 1, static_cast<size_t>(1));
    size_t mip_height = std::max(parent_height >> 1, static_cast<size_t>(1));
    size_t num_bytes = CalculateSize(GLMipMap::kRGBA, mip_width, mip_height);
    GLMipMap::MipPointer mip(base::AlignedAlloc(num_bytes, 16),
                             base::AlignedFreeDeleter());

    // If the width isn't perfectly divisable with two, then we end up skewing
    // the image because the source offset isn't updated properly.
    bool unaligned_width = parent_width & 1;

    // Special case the non-uniform/anisotropic cases, eg 4:1 or 1:4 textures.
    // This is only an issue once we reach the highest mip levels where one
    // dimension is one pixel.
    if (parent_width == 1) {
      // Interestingly the horizontal and vertical case becomes the same code,
      // it's only about which value to use as the run length that differs.
      MipNonUniform(mip.get(), parent_mip.get(), mip_height);
    } else if (parent_height == 1) {
      MipNonUniform(mip.get(), parent_mip.get(), mip_width);
    } else {
      const uint32_t* s = reinterpret_cast<const uint32_t*>(parent_mip.get());
      uint32_t* d = reinterpret_cast<uint32_t*>(mip.get());
      for (size_t y = 0; y < mip_height; ++y) {
        for (size_t x = 0; x < mip_width; ++x) {
          *d++ = Mix4(s[0], s[1], s[parent_width], s[parent_width + 1]);
          s += 2;
        }
        if (unaligned_width)
          ++s;
        s += parent_width;
      }
    }

    parent_width = mip_width;
    parent_height = mip_height;
    parent_mip = mip;

    if (encoder) {
      // Compress the mip level.
      size_t num_bytes = CalculateSize(format, mip_width, mip_height);
      GLMipMap::MipPointer compressed_mip(base::AlignedAlloc(num_bytes, 16),
                                          base::AlignedFreeDeleter());
      // TODO(peterp): Should probably use low quality for small mip levels.
      encoder->Compress(reinterpret_cast<const uint8_t*>(mip.get()),
                        reinterpret_cast<uint8_t*>(compressed_mip.get()),
                        mip_width, mip_height,
                        cc::TextureCompressor::kQualityHigh);
      result.push_back(compressed_mip);
    } else {
      result.push_back(mip);
    }
  }
  return result;
}

// Round the value both up and down to the power of two, then compare the
// difference from the original value and return the closest one.
size_t FindClosestPOT(size_t value) {
  return SkNextPow2((value + 1) * 2 / 3);
}

// Rescale a bitmap with bi-linear filtering.
void ScaleBitmap(SkBitmap* dst,
                 const SkBitmap& src,
                 size_t new_width,
                 size_t new_height) {
  // If this fails then we want to crash.
  CHECK(dst->tryAllocPixels(SkImageInfo::MakeN32Premul(new_width, new_height)));
  dst->eraseARGB(0, 0, 0, 0);

  // Use the skia blitter with a transform.
  SkCanvas canvas(*dst);
  canvas.scale(new_width / static_cast<float>(src.width()),
               new_height / static_cast<float>(src.height()));
  canvas.drawBitmap(src, 0, 0, NULL);
}

}  // namespace

GLMipMap::GLMipMap(size_t width,
                   size_t height,
                   size_t original_width,
                   size_t original_height,
                   size_t original_levels,
                   Format format,
                   MipVector mips)
    : width_(width),
      height_(height),
      original_width_(original_width),
      original_height_(original_height),
      original_levels_(original_levels),
      format_(format),
      mips_(std::move(mips)) {
  DCHECK(!mips_.empty());
  DCHECK_GE(original_levels_, mips_.size());
}

std::unique_ptr<GLMipMap> GLMipMap::Create(jobject bitmap,
                                           bool alpha,
                                           CompressionStrategy strategy) {
  CHECK(bitmap);

  gfx::JavaBitmap java_bitmap(bitmap);
  CHECK_EQ(java_bitmap.format(), ANDROID_BITMAP_FORMAT_RGBA_8888);
  gfx::Size src_size = java_bitmap.size();

  // Wrap up the raw image data in a skia bitmap structure.
  SkBitmap skia_bitmap;
  skia_bitmap.setInfo(
      SkImageInfo::MakeN32Premul(src_size.width(), src_size.height()));
  skia_bitmap.setPixels(java_bitmap.pixels());

  return Create(skia_bitmap, alpha, strategy);
}

std::unique_ptr<GLMipMap> GLMipMap::Create(const SkBitmap& bitmap,
                                           bool alpha,
                                           CompressionStrategy strategy) {
  CHECK(!bitmap.empty());
  CHECK_EQ(bitmap.colorType(), kRGBA_8888_SkColorType);

  size_t width = bitmap.width();
  size_t height = bitmap.height();
  Format format = GetBestCompressedFormat(width, height, alpha, strategy);

  SkAutoLockPixels pixels_lock(bitmap);
  void* pixels = bitmap.getPixels();

  // Adjust the size of the bitmap if the hardware can't support NPOT.
  SkBitmap scaled_bitmap;
  if (!GLLibrary::Get().SupportsNPOT()) {
    if (!SkIsPow2(width) || !SkIsPow2(height)) {
      // Figure out a size that we can use instead.
      width = FindClosestPOT(width);
      height = FindClosestPOT(height);

      // Bi-linear rescaling and update the actual dimension.
      ScaleBitmap(&scaled_bitmap, bitmap, width, height);
      pixels = scaled_bitmap.getPixels();
    }
  }

  MipVector mips = CreateMips(width, height, pixels, format);
  size_t original_levels = mips.size();

  return std::unique_ptr<GLMipMap>(
      new GLMipMap(width, height, bitmap.width(), bitmap.height(),
                   original_levels, format, std::move(mips)));
}

std::unique_ptr<GLMipMap> GLMipMap::CreateReduced(size_t levels) const {
  size_t baseline = levels < mips_.size() ? mips_.size() - levels : 0;
  return std::unique_ptr<GLMipMap>(
      new GLMipMap(width(baseline), height(baseline), original_width_,
                   original_height_, original_levels_, format_,
                   MipVector(mips_.begin() + baseline, mips_.end())));
}

size_t GLMipMap::ReduceSize(size_t base_size, size_t level) const {
  return base_size > 1UL << level ? base_size >> level : 1;
}

size_t GLMipMap::width(size_t level) const {
  return ReduceSize(width_, level);
}

size_t GLMipMap::height(size_t level) const {
  return ReduceSize(height_, level);
}

size_t GLMipMap::original_width(size_t level) const {
  return ReduceSize(original_width_, level);
}

size_t GLMipMap::original_height(size_t level) const {
  return ReduceSize(original_height_, level);
}

size_t GLMipMap::levels() const {
  return mips_.size();
}

const void* GLMipMap::getPixels(size_t level) const {
  return mips_.at(level).get();
}

size_t GLMipMap::GetSize(size_t level) const {
  return CalculateSize(format_, width(level), height(level));
}

GLMipMap::Format GLMipMap::GetBestCompressedFormat(
    size_t width,
    size_t height,
    bool alpha,
    CompressionStrategy strategy) {
  if (strategy == kCompressNever)
    return alpha ? kRGBA : kRGB;

  // Detect the best format for the current graphics card.
  const GLLibrary& gl = GLLibrary::Get();
  if (!gl.initialized())
    LOG(WARNING) << "GL not initialized, using uncompressed format";

  if (alpha) {
    if (gl.SupportsATC())
      return kATCIA;
    if (gl.SupportsDXT5())
      return kDXT5;
    return kRGBA;
  }

  if (gl.SupportsATC())
    return kATC;
  if (gl.SupportsDXT1())
    return kDXT1;
  if (gl.SupportsETC1() &&
      (strategy == kCompressAlways || !IsVeryLargeSize(width, height)))
    return kETC1;
  return kRGB;
}

bool GLMipMap::IsVeryLargeSize(size_t width, size_t height) {
  const size_t kLargeWidth = 1024;
  const size_t kLargeHeight = 1280;

  return width * height > kLargeWidth * kLargeHeight;
}

// Increment this version number when the file format is changed.
const size_t GLMipMap::kFileVersion = 3;

struct GLMipMap::BufferSizes {
  int uncompressed;
  int compressed;
};

struct GLMipMap::FileHeader {
  size_t version;
  size_t width;
  size_t height;
  size_t original_width;
  size_t original_height;
  size_t original_levels;
  Format format;
};

size_t GLMipMap::GetFileVersion(bool is_rtl) {
  // Encode language dependent right-to-left mode into stored version to prevent
  // loading mipmaps saved in different rtl mode. Otherwise we could show a tab
  // with a left side close button, but listen for click events on right side or
  // vice versa.
  return 2 * GLMipMap::kFileVersion + is_rtl;
}

bool GLMipMap::SaveToFile(const std::string& file_name, bool is_rtl) const {
  // Compute size before compression.
  BufferSizes sizes;
  sizes.uncompressed = sizeof(GLMipMap::FileHeader);
  for (size_t level = 0; level < levels(); ++level)
    sizes.uncompressed += GetSize(level);

  std::unique_ptr<char> buf(new char[sizes.uncompressed]);
  char* bufp = buf.get();

  // Write header to uncompressed buffer.
  GLMipMap::FileHeader header;
  header.version = GetFileVersion(is_rtl);
  header.width = width_;
  header.height = height_;
  header.original_width = original_width_;
  header.original_height = original_height_;
  header.original_levels = original_levels_;
  header.format = format_;
  memcpy(bufp, &header, sizeof(GLMipMap::FileHeader));
  bufp += sizeof(GLMipMap::FileHeader);

  // Write each level to uncompressed buffer.
  for (size_t level = 0; level < levels(); ++level) {
    size_t level_size = GetSize(level);
    memcpy(bufp, getPixels(level), level_size);
    bufp += level_size;
  }

  DCHECK_EQ(bufp, buf.get() + sizes.uncompressed);

  // Compress to lz_buf.
  std::unique_ptr<char> lz_buf(new char[LZ4_compressBound(sizes.uncompressed)]);
  sizes.compressed = LZ4_compress(buf.get(), lz_buf.get(), sizes.uncompressed);
  buf.reset();

  if (!sizes.compressed) {
    LOG(ERROR) << "Failed to compress block of size " << sizes.uncompressed;
    return false;
  }

  FILE* fp = fopen(file_name.c_str(), "w");
  if (!fp) {
    LOG(ERROR) << "Failed to create thumbnail file \"" << file_name
               << "\" errno=" << errno;
    return false;
  }

  // Write size before and after compression, followed by compressed data.
  if (fwrite(&sizes, sizeof(sizes), 1, fp) != 1) {
    LOG(ERROR) << "Failed to write sizes to thumbnail file " << file_name;
    fclose(fp);
    return false;
  }

  // Write compressed data.
  if (fwrite(lz_buf.get(), sizes.compressed, 1, fp) != 1) {
    LOG(ERROR) << "Failed to write compressed data to thumbnail file "
               << file_name;
    fclose(fp);
    return false;
  }

  fclose(fp);
  return true;
}

std::unique_ptr<GLMipMap> GLMipMap::CreateFromFile(const std::string& file_name,
                                                   bool is_rtl) {
  FILE* fp = fopen(file_name.c_str(), "r");
  if (!fp) {
    LOG(ERROR) << "Failed to open thumbnail file \"" << file_name
               << "\" errno=" << errno;
    return nullptr;
  }

  // Read size before and after compression.
  BufferSizes sizes;
  if (fread(&sizes, sizeof(sizes), 1, fp) != 1) {
    LOG(ERROR) << "Failed to read sizes from thumbnail file " << file_name;
    fclose(fp);
    return nullptr;
  }

  // Check that stored size is plausible to not terminate from uncaught bad
  // alloc in case of a corrupted file.
  if (sizes.compressed <= 0 || sizes.compressed > 10000000) {
    LOG(ERROR) << "Unexpected file size " << sizes.compressed;
    fclose(fp);
    return nullptr;
  }

  // Read compressed data.
  std::unique_ptr<char> lz_buf(new char[sizes.compressed]);
  if (fread(lz_buf.get(), sizes.compressed, 1, fp) != 1) {
    LOG(ERROR) << "Failed to read compressed data from thumbnail file "
               << file_name;
    fclose(fp);
    return nullptr;
  }

  // Decompress data into buf.
  std::unique_ptr<char> buf(new char[sizes.uncompressed]);
  if (LZ4_decompress_safe(lz_buf.get(), buf.get(), sizes.compressed,
                          sizes.uncompressed) != sizes.uncompressed) {
    LOG(ERROR) << "Failed to decompress block of size " << sizes.compressed;
    fclose(fp);
    return nullptr;
  }
  fclose(fp);
  lz_buf.reset();

  // Header is located first in uncompressed buffer.
  char* bufp = buf.get();
  GLMipMap::FileHeader* header = reinterpret_cast<GLMipMap::FileHeader*>(bufp);
  bufp += sizeof(GLMipMap::FileHeader);
  if (header->version != GetFileVersion(is_rtl)) {
    LOG(WARNING) << "Will not read thumbnail from file version "
                 << header->version << ", expected file version "
                 << GetFileVersion(is_rtl);
    return nullptr;
  }

  // Restore each level from uncompressed buffer.
  size_t width = header->width;
  size_t height = header->height;
  GLMipMap::MipVector mips;
  do {
    size_t level_size = CalculateSize(header->format, width, height);
    mips.emplace_back(new char[level_size]);
    memcpy(mips.back().get(), bufp, level_size);
    bufp += level_size;
    width = std::max(width / 2, static_cast<size_t>(1));
    height = std::max(height / 2, static_cast<size_t>(1));
  } while (bufp != buf.get() + sizes.uncompressed);

  return std::unique_ptr<GLMipMap>(
      new GLMipMap(header->width, header->height, header->original_width,
                   header->original_height, header->original_levels,
                   header->format, std::move(mips)));
}

// TODO(ollel): Remove when support for old file format is dropped.
std::unique_ptr<GLMipMap> GLMipMap::CreateFromLegacyFile(
    const std::string& file_name) {
  FILE* fp = fopen(file_name.c_str(), "r");
  if (!fp) {
    LOG(ERROR) << "Failed to open legacy thumbnail file \"" << file_name
               << "\" errno=" << errno;
    return nullptr;
  }

  // Read size of jpeg data stored from Java, always in big endian.
  uint32_t size = 0;
  for (int i = 0; i < 4; ++i) {
    uint8_t byte;
    if (fread(&byte, sizeof(byte), 1, fp) != 1) {
      LOG(ERROR) << "Failed to read size from legacy thumbnail file "
                 << file_name;
      fclose(fp);
      return nullptr;
    }
    size = size * 256 + byte;
  }

  // Check that stored size is plausible to not terminate from uncaught bad
  // alloc in case of a corrupted file.
  if (size > 1000000) {
    LOG(ERROR) << "Unexpected size " << size << " of legacy thumbnail file "
               << file_name;
    fclose(fp);
    return nullptr;
  }

  // Read jpeg data and decode into bitmap.
  std::unique_ptr<unsigned char> buf(new unsigned char[size]);
  if (fread(buf.get(), size, 1, fp) != 1) {
    LOG(ERROR) << "Failed to read data from legacy thumbnail file "
               << file_name;
    fclose(fp);
    return nullptr;
  }
  fclose(fp);

  std::vector<unsigned char> bitmap_data;
  int bitmap_width;
  int bitmap_height;
  if (!gfx::JPEGCodec::Decode(buf.get(), size, gfx::JPEGCodec::FORMAT_SkBitmap,
                              &bitmap_data, &bitmap_width, &bitmap_height)) {
    LOG(ERROR) << "Failed to decode legacy thumbnail file " << file_name;
    return nullptr;
  }
  buf.reset();

  SkBitmap bitmap;
  if (!bitmap.tryAllocN32Pixels(bitmap_width, bitmap_height)) {
    LOG(ERROR) << "Failed to allocate legacy thumbnail image for file "
               << file_name;
    return nullptr;
  }
  memcpy(bitmap.getAddr32(0, 0), &bitmap_data[0],
         bitmap_width * bitmap_height * 4);

  return Create(bitmap, false, kCompressAlways);
}

}  // namespace opera
