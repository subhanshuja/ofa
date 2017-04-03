// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BITMAP_OP_SCANLINE_OPS_H_
#define COMMON_BITMAP_OP_SCANLINE_OPS_H_

#ifdef __cplusplus
extern "C" {
#endif

// Halve the size in X and Y using a box filter (i.e. each output pixel is the
// average of four input pixels).
void HalveXY_ARGB8888(void* dst, void* src1, void* src2, int count);
void HalveXY_RGB565(void* dst, void* src1, void* src2, int count);

// Linear interpolation along the X-axis.
void LerpX_ARGB8888(void* dst, void* src, int count, float step);
void LerpX_RGB565(void* dst, void* src, int count, float step);

// Linear interpolation along the Y-axis.
void LerpY_ARGB8888(void* dst, void* src1, void* src2, int count, float y_pos);
void LerpY_RGB565(void* dst, void* src1, void* src2, int count, float y_pos);

typedef struct {
  int a;
  int r;
  int g;
  int b;
} Color128;

// Sum of colors.
void ColorSum_ARGB8888(void* row, int count, Color128* sum);
void ColorSum_RGB565(void* row, int count, Color128* sum);

// Color histogram (separate histograms for R, G and B).
void ColorHist_ARGB8888(void* row, int count, int* hist_r, int* hist_g,
    int* hist_b);
void ColorHist_RGB565(void* row, int count, int* hist_r, int* hist_g,
    int* hist_b);

#ifdef __cplusplus
}
#endif

#endif  // COMMON_BITMAP_OP_SCANLINE_OPS_H_
