// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OPENGL_UTILS_GL_MIPMAP_H_
#define COMMON_OPENGL_UTILS_GL_MIPMAP_H_

#include <jni.h>

#include <memory>
#include <string>
#include <vector>

class SkBitmap;

namespace opera {

// A two-dimensional image that can be used as an OpenGL texture.
//
// It can hold a complete mip chain in a bitmap format that may be compressed.
class GLMipMap {
 public:
  enum Format {
    // Uncompressed formats.
    kRGB,
    kRGBA,
    // Compressed formats.
    kATC,
    kATCIA,
    kDXT1,
    kDXT5,
    kETC1
  };

  enum CompressionStrategy { kCompressNever, kCompressIfFast, kCompressAlways };

  typedef std::shared_ptr<void> MipPointer;
  typedef std::vector<MipPointer> MipVector;

  // Create a new GLMipMap from a java bitmap.
  static std::unique_ptr<GLMipMap> Create(jobject bitmap,
                                          bool alpha,
                                          CompressionStrategy strategy);

  // Create a new GLMipMap from a skia bitmap.
  static std::unique_ptr<GLMipMap> Create(const SkBitmap& bitmap,
                                          bool alpha,
                                          CompressionStrategy strategy);

  // Restore from file. Returns a new GLMipMap or null in case of failure.
  static std::unique_ptr<GLMipMap> CreateFromFile(const std::string& file_name,
                                                  bool is_rtl);

  // Restore from file in old format. Returns a new GLMipMap or null in case of
  // failure.
  // TODO(ollel): Remove when support for old file format is dropped.
  static std::unique_ptr<GLMipMap> CreateFromLegacyFile(
      const std::string& file_name);

  // Create a copy with reduced number of levels.
  std::unique_ptr<GLMipMap> CreateReduced(size_t levels) const;

  // Save to file. Returns true on success or false on failure.
  bool SaveToFile(const std::string& file_name, bool is_rtl) const;

  // Parameter access.

  // The actual dimensions of the image data. This may have been adjusted to be
  // a power of two if the GL driver doesn't have NPOT support.
  size_t width() const { return width_; }
  size_t height() const { return height_; }
  size_t width(size_t level) const;
  size_t height(size_t level) const;

  // The dimensions of the image data when it was created.
  size_t original_width() const { return original_width_; }
  size_t original_height() const { return original_height_; }
  size_t original_width(size_t level) const;
  size_t original_height(size_t level) const;

  Format format() const { return format_; }
  size_t levels() const;
  size_t original_levels() const { return original_levels_; }

  const void* getPixels(size_t level) const;

  size_t GetSize(size_t level) const;

  bool IsCompressed() const { return format_ >= kATC; }

 private:
  struct BufferSizes;
  struct FileHeader;
  static const size_t kFileVersion;

  GLMipMap(size_t width,
           size_t height,
           size_t original_width,
           size_t original_height,
           size_t original_levels,
           Format format,
           MipVector mips);

  static Format GetBestCompressedFormat(size_t width,
                                        size_t height,
                                        bool alpha,
                                        CompressionStrategy strategy);
  static size_t GetFileVersion(bool is_rtl);

  // Tests if an image size is very large.
  static bool IsVeryLargeSize(size_t width, size_t height);

  size_t ReduceSize(size_t base_size, size_t level) const;

  const size_t width_;
  const size_t height_;
  const size_t original_width_;
  const size_t original_height_;
  const size_t original_levels_;
  const Format format_;
  const MipVector mips_;
};

}  // namespace opera

#endif  // COMMON_OPENGL_UTILS_GL_MIPMAP_H_
