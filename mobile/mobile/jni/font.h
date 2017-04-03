/*
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#ifndef ANDROID_FONT_H
#define ANDROID_FONT_H

#include <jni.h>

#include "bream/vm/c/graphics/bream_graphics.h"

OP_EXTERN_C_BEGIN
#include "mini/c/shared/core/font/FontsCollection.h"
#include "mini/c/shared/generic/vm_integ/VMInstance.h"
OP_EXTERN_C_END

class AndroidFont
{
 public:
  AndroidFont();
  ~AndroidFont();

  static bool InitGlobals();

  bool Init(bool serif, bool monospace, bool bold, bool italic,
            unsigned int size);

  static AndroidFont* GetFont(UINT16 obml_font);
  static AndroidFont* GetFont(MOpFont* font);
  static int GetFontSize(unsigned int index);
  static size_t GetFontCount();
  static int GetDefaultFontIndex(unsigned int textSize);
  static int GetFontScale(unsigned int textSize);

  const char* GetName() const;

  unsigned int GetHeight() const {
    return GetHeight(1, 1);
  }
  unsigned int GetHeight(int unscaled, int scaled) const;

  unsigned int StringWidth(const char* text, size_t length) const {
    return StringWidth(text, length, 1, 1);
  }
  unsigned int StringWidth(const char* text,
                           size_t length,
                           int unscaled,
                           int scaled) const;

  unsigned int StringWidth(const WCHAR* text, size_t length) const {
    return StringWidth(text, length, 1, 1);
  }
  unsigned int StringWidth(const WCHAR* text,
                           size_t length,
                           int unscaled,
                           int scaled) const;

  void CharactersWidth(const WCHAR* text,
                       size_t length,
                       UINT16* widths,
                       int unscaled,
                       int scaled) const;

  void DrawString(bgContext* context,
                  const char* text,
                  size_t length,
                  int x,
                  int y,
                  BG_BLEND blend) const {
    DrawString(context, text, length, x, y, 1, 1, blend);
  }
  void DrawString(bgContext* context,
                  const char* text,
                  size_t length,
                  int x,
                  int y,
                  int unscaled,
                  int scaled,
                  BG_BLEND blend) const;

  static MOP_STATUS CreateCollection(MOpFontsCollection** collection);

  bool serif() const { return serif_; }
  bool monospace() const { return monospace_; }
  bool bold() const { return bold_; }
  bool italic() const { return italic_; }
  unsigned int ascent() const { return ascent_; }
  unsigned int descent() const { return descent_; }

  jobject paint() { return paint_; }

 private:
  static bool CreateFonts();
  static bool CreateFontForAllSizes(bool serif, bool monospace, bool bold,
                                    bool italic);

  bool InitPaint(bool serif, bool monospace, bool bold, bool italic,
                 unsigned int size);
  bool InitMetrics();

  jobject paint_;
  bool serif_;
  bool monospace_;
  bool bold_;
  bool italic_;
  unsigned int ascent_;
  unsigned int descent_;
};

#endif /* ANDROID_FONT_H */
