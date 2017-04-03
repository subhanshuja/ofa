// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bitmap/op_scanline_ops.h"

#include <stdint.h>

// The ARGB8888 format is as follows (binary):
//  AAAAAAAABBBBBBBBGGGGGGGGRRRRRRRR
#define MASK_8888_BR 0x00ff00ff
#define MASK_8888_AG 0xff00ff00

// The RGB565 format is as follows (binary):
//  RRRRRGGGGGGBBBBB
#define MASK_565_RB 0xf81f
#define MASK_565_G  0x07e0

void HalveXY_ARGB8888(void* dst, void* src1, void* src2, int count) {
  uint32_t* d = (uint32_t*)(dst);
  uint32_t* s1 = (uint32_t*)(src1);
  uint32_t* s2 = (uint32_t*)(src2);
  while (count--) {
    uint32_t c1 = s1[0];
    uint32_t c2 = s1[1];
    s1 += 2;
    uint32_t c3 = s2[0];
    uint32_t c4 = s2[1];
    s2 += 2;

    uint32_t c = (((c1 & MASK_8888_BR) + (c2 & MASK_8888_BR) +
                   (c3 & MASK_8888_BR) + (c4 & MASK_8888_BR)) >> 2) & MASK_8888_BR;
    c1 &= MASK_8888_AG;
    c2 &= MASK_8888_AG;
    c3 &= MASK_8888_AG;
    c4 &= MASK_8888_AG;
    c |= ((c1 >> 2) + (c2 >> 2) + (c3 >> 2) + (c4 >> 2)) & MASK_8888_AG;

    *d++ = c;
  }
}

void HalveXY_RGB565(void* dst, void* src1, void* src2, int count) {
  uint16_t* d = (uint16_t*)(dst);
  uint16_t* s1 = (uint16_t*)(src1);
  uint16_t* s2 = (uint16_t*)(src2);
  while (count--) {
    uint32_t c1 = s1[0];
    uint32_t c2 = s1[1];
    s1 += 2;
    uint32_t c3 = s2[0];
    uint32_t c4 = s2[1];
    s2 += 2;

    *d++ = ((((c1 & MASK_565_RB) + (c2 & MASK_565_RB) +
              (c3 & MASK_565_RB) + (c4 & MASK_565_RB)) >> 2) & MASK_565_RB) |
           ((((c1 & MASK_565_G) + (c2 & MASK_565_G) +
              (c3 & MASK_565_G) + (c4 & MASK_565_G)) >> 2) & MASK_565_G);
  }
}

void LerpX_ARGB8888(void* dst, void* src, int count, float step) {
  uint32_t* d = (uint32_t*)(dst);
  uint32_t* s = (uint32_t*)(src);
  uint32_t pos = 0;
  uint32_t incr = (uint32_t)(step * 256.0f);
  while (count--) {
    uint32_t idx = pos >> 8;
    uint16_t w2 = pos & 0xff;
    uint16_t w1 = 0x100 - w2;
    pos += incr;

    uint32_t c1 = s[idx];
    uint32_t c2 = s[idx + 1];

    *d++ = (((w1 * (c1 & MASK_8888_BR) + w2 * (c2 & MASK_8888_BR)) >> 8) & MASK_8888_BR) |
           (((w1 * ((c1 >> 8) & MASK_8888_BR) + w2 * ((c2 >> 8) & MASK_8888_BR))) & MASK_8888_AG);
  }
}

void LerpX_RGB565(void* dst, void* src, int count, float step) {
  uint16_t* d = (uint16_t*)(dst);
  uint16_t* s = (uint16_t*)(src);
  uint32_t pos = 0;
  uint32_t incr = (uint32_t)(step * 256.0f);
  while (count--) {
    uint32_t idx = pos >> 8;
    uint16_t w2 = (pos >> 2) & 0x3f;
    uint16_t w1 = 0x40 - w2;
    pos += incr;

    uint32_t c1 = s[idx];
    uint32_t c2 = s[idx + 1];

    *d++ = (((w1 * (c1 & MASK_565_G) + w2 * (c2 & MASK_565_G)) >> 6) & MASK_565_G) |
           (((w1 * (c1 & MASK_565_RB) + w2 * (c2 & MASK_565_RB)) >> 6) & MASK_565_RB);
  }
}

void LerpY_ARGB8888(void* dst, void* src1, void* src2, int count, float y_pos) {
  uint32_t* d = (uint32_t*)(dst);
  uint32_t* s1 = (uint32_t*)(src1);
  uint32_t* s2 = (uint32_t*)(src2);
  uint16_t w2 = (uint32_t)(y_pos * 256.0f) & 0xff;
  uint16_t w1 = 0x100 - w2;
  while (count--) {
    uint32_t c1 = *s1++;
    uint32_t c2 = *s2++;

    *d++ = (((w1 * (c1 & MASK_8888_BR) + w2 * (c2 & MASK_8888_BR)) >> 8) & MASK_8888_BR) |
           (((w1 * ((c1 >> 8) & MASK_8888_BR) + w2 * ((c2 >> 8) & MASK_8888_BR))) & MASK_8888_AG);
  }
}

void LerpY_RGB565(void* dst, void* src1, void* src2, int count, float y_pos) {
  uint16_t* d = (uint16_t*)(dst);
  uint16_t* s1 = (uint16_t*)(src1);
  uint16_t* s2 = (uint16_t*)(src2);
  uint16_t w2 = (uint32_t)(y_pos * 64.0f) & 0x3f;
  uint16_t w1 = 0x40 - w2;
  while (count--) {
    uint32_t c1 = *s1++;
    uint32_t c2 = *s2++;

    *d++ = (((w1 * (c1 & MASK_565_G) + w2 * (c2 & MASK_565_G)) >> 6) & MASK_565_G) |
           (((w1 * (c1 & MASK_565_RB) + w2 * (c2 & MASK_565_RB)) >> 6) & MASK_565_RB);
  }
}

void ColorSum_ARGB8888(void* row, int count, Color128* sum) {
  int a = 0,  r = 0,  g = 0,  b = 0;
  uint32_t* ptr = (uint32_t*)(row);

  // Do unrolled packed addition first.
  while (count >= 4) {
    // Sum at most 256 samples (to avoid 16-bit overflow). Also, we must
    // terminate the unrolled loop when there are less than 4 pixels left.
    int count_low = count - 255;
    if (count_low < 4) {
      count_low = 4;
    }
    uint32_t ag = 0, br = 0, c;
    for (; count >= count_low; count -= 4) {
      c = *ptr++;
      ag += (c >> 8) & MASK_8888_BR;
      br += c & MASK_8888_BR;
      c = *ptr++;
      ag += (c >> 8) & MASK_8888_BR;
      br += c & MASK_8888_BR;
      c = *ptr++;
      ag += (c >> 8) & MASK_8888_BR;
      br += c & MASK_8888_BR;
      c = *ptr++;
      ag += (c >> 8) & MASK_8888_BR;
      br += c & MASK_8888_BR;
    }
    a += ag >> 16;      // The upper 16 bits hold the sum of the A component.
    r += br & 0xffff;   // The lower 16 bits hold the sum of the R component.
    g += ag & 0xffff;   // The lower 16 bits hold the sum of the G component.
    b += br >> 16;      // The upper 16 bits hold the sum of the B component.
  }

  // Tail loop.
  while (count--) {
    uint32_t c = *ptr++;
    a += c >> 24;
    r += c & 0xff;
    g += (c >> 8) & 0xff;
    b += (c >> 16) & 0xff;
  }

  sum->a += a;
  sum->r += r;
  sum->g += g;
  sum->b += b;
}

void ColorSum_RGB565(void* row, int count, Color128* sum) {
  // Alpha is always 255 for RGB565.
  sum->a += 255 * count;

  // Individually sum R, G and B components for all pixels.
  int r = 0,  g = 0,  b = 0;
  uint16_t* ptr = (uint16_t*)(row);
  while (count--) {
    uint32_t c = *ptr++;
    r += c >> 11;         // Upper 5 bits = red.
    g += (c >> 5) & 0x3f; // Middle 6 bits = green.
    b += c & 0x1f;        // Lower 5 bits = blue.
  }

  // Scale 5-bit and 6-bit color components to mimic results based on 8-bit
  // color components.
  sum->r += (r * 255) / 31;
  sum->g += (g * 255) / 63;
  sum->b += (b * 255) / 31;
}

void ColorHist_ARGB8888(void* row, int count, int* hist_r, int* hist_g,
    int* hist_b) {
  uint32_t* ptr = (uint32_t*)(row);
  while (count--) {
    uint32_t c = *ptr++;
    hist_r[c & 0xff]++;
    hist_g[(c >> 8) & 0xff]++;
    hist_b[(c >> 16) & 0xff]++;
  }
}

void ColorHist_RGB565(void* row, int count, int* hist_r, int* hist_g,
    int* hist_b) {
  uint16_t* ptr = (uint16_t*)(row);
  while (count--) {
    uint32_t c = *ptr++;

    // Extract color components.
    int r = c >> 11;         // Upper 5 bits = red.
    int g = (c >> 5) & 0x3f; // Middle 6 bits = green.
    int b = c & 0x1f;        // Lower 5 bits = blue.

    // Scale color components to 8-bit resolution (0-255).
    // These are quick approximations of 255*x/31 and 255*x/63.
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);

    // Update histogram.
    hist_r[r]++;
    hist_g[g]++;
    hist_b[b]++;
  }
}
