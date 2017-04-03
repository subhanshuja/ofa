// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/compiler_specific.h"
#include "skia/ext/analysis_canvas.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkShader.h"
#include "third_party/skia/include/effects/SkGradientShader.h"
#include "third_party/skia/include/effects/SkOffsetImageFilter.h"

namespace {

void SolidColorFill(skia::AnalysisCanvas& canvas) {
  canvas.clear(SkColorSetARGB(255, 255, 255, 255));
}

void TransparentFill(skia::AnalysisCanvas& canvas) {
  canvas.clear(SkColorSetARGB(0, 0, 0, 0));
}

} // namespace
namespace skia {

TEST(AnalysisCanvasTest, EmptyCanvas) {
  int flags = skia::AnalysisCanvas::ANALYZE_SOLID_COLOR |
              skia::AnalysisCanvas::ANALYZE_LOSSLESS_BITMAP |
              skia::AnalysisCanvas::ANALYZE_GRADIENT;
  skia::AnalysisCanvas canvas(255, 255, flags);

  SkColor color;
  EXPECT_TRUE(canvas.GetColorIfSolid(&color));
  EXPECT_EQ(color, SkColorSetARGB(0, 0, 0, 0));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());
}

TEST(AnalysisCanvasTest, ClearCanvas) {
  int flags = skia::AnalysisCanvas::ANALYZE_SOLID_COLOR |
              skia::AnalysisCanvas::ANALYZE_LOSSLESS_BITMAP |
              skia::AnalysisCanvas::ANALYZE_GRADIENT;
  skia::AnalysisCanvas canvas(255, 255, flags);

  // Transparent color
  SkColor color = SkColorSetARGB(0, 12, 34, 56);
  canvas.clear(color);

  SkColor outputColor;
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  // Solid color
  color = SkColorSetARGB(255, 65, 43, 21);
  canvas.clear(color);

  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_EQ(outputColor, color);
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  // Translucent color
  color = SkColorSetARGB(128, 11, 22, 33);
  canvas.clear(color);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  // Test helper methods
  SolidColorFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  TransparentFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());
}

TEST(AnalysisCanvasTest, ComplexActions) {
  int flags = skia::AnalysisCanvas::ANALYZE_SOLID_COLOR |
              skia::AnalysisCanvas::ANALYZE_LOSSLESS_BITMAP |
              skia::AnalysisCanvas::ANALYZE_GRADIENT;
  skia::AnalysisCanvas canvas(255, 255, flags);

  // Draw paint test.
  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  SkPaint paint;
  paint.setColor(color);

  canvas.drawPaint(paint);

  SkColor outputColor;
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  // Draw points test.
  SkPoint points[4] = {
    SkPoint::Make(0, 0),
    SkPoint::Make(255, 0),
    SkPoint::Make(255, 255),
    SkPoint::Make(0, 255)
  };

  SolidColorFill(canvas);
  canvas.drawPoints(SkCanvas::kLines_PointMode, 4, points, paint);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  // Draw oval test.
  SolidColorFill(canvas);
  canvas.drawOval(SkRect::MakeWH(255, 255), paint);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  // Draw gradient oval test.
  SolidColorFill(canvas);
  SkColor gradientColors[2] = {SK_ColorWHITE, SK_ColorBLACK};
  SkPoint gradientPoints[2];
  gradientPoints[0].iset(0, 0);
  gradientPoints[1].iset(100, 0);
  skia::RefPtr<SkShader> shader(skia::AdoptRef(SkGradientShader::CreateLinear(
      gradientPoints, gradientColors, NULL, 2, SkShader::kClamp_TileMode)));
  SkPaint gradientPaint;
  gradientPaint.setShader(shader.get());
  canvas.drawOval(SkRect::MakeWH(255, 255), gradientPaint);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_TRUE(canvas.HasGradient());

  // Draw bitmap test.
  SolidColorFill(canvas);
  SkBitmap secondBitmap;
  secondBitmap.allocN32Pixels(255, 255);
  canvas.drawBitmap(secondBitmap, 0, 0);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  // Draw small lossless bitmap test.
  SolidColorFill(canvas);
  SkBitmap losslessSmallBitmap;
  losslessSmallBitmap.allocN32Pixels(32, 32);
  losslessSmallBitmap.setIsDecodedWithLosslessCodec(true);
  canvas.drawBitmap(losslessSmallBitmap, 0, 0);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  // Draw large lossless bitmap test.
  SolidColorFill(canvas);
  SkBitmap losslessLargeBitmap;
  losslessLargeBitmap.allocN32Pixels(33, 33);
  losslessLargeBitmap.setIsDecodedWithLosslessCodec(true);
  canvas.drawBitmap(losslessLargeBitmap, 0, 0);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_TRUE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());
}

TEST(AnalysisCanvasTest, SimpleDrawRect) {
  int flags = skia::AnalysisCanvas::ANALYZE_SOLID_COLOR |
              skia::AnalysisCanvas::ANALYZE_LOSSLESS_BITMAP |
              skia::AnalysisCanvas::ANALYZE_GRADIENT;
  skia::AnalysisCanvas canvas(255, 255, flags);

  SkColor color = SkColorSetARGB(255, 11, 22, 33);
  SkPaint paint;
  paint.setColor(color);
  canvas.clipRect(SkRect::MakeWH(255, 255));
  canvas.drawRect(SkRect::MakeWH(255, 255), paint);

  SkColor outputColor;
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_EQ(color, outputColor);
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  color = SkColorSetARGB(255, 22, 33, 44);
  paint.setColor(color);
  canvas.translate(-128, -128);
  canvas.drawRect(SkRect::MakeWH(382, 382), paint);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  color = SkColorSetARGB(255, 33, 44, 55);
  paint.setColor(color);
  canvas.drawRect(SkRect::MakeWH(383, 383), paint);

  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_EQ(color, outputColor);
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  color = SkColorSetARGB(0, 0, 0, 0);
  paint.setColor(color);
  canvas.drawRect(SkRect::MakeWH(383, 383), paint);

  // This test relies on canvas treating a paint with 0-color as a no-op
  // thus not changing its "is_solid" status.
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_EQ(outputColor, SkColorSetARGB(255, 33, 44, 55));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  color = SkColorSetARGB(128, 128, 128, 128);
  paint.setColor(color);
  canvas.drawRect(SkRect::MakeWH(383, 383), paint);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  paint.setXfermodeMode(SkXfermode::kClear_Mode);
  canvas.drawRect(SkRect::MakeWH(382, 382), paint);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  canvas.drawRect(SkRect::MakeWH(383, 383), paint);

  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  canvas.translate(128, 128);
  color = SkColorSetARGB(255, 11, 22, 33);
  paint.setColor(color);
  paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);
  canvas.drawRect(SkRect::MakeWH(255, 255), paint);

  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  EXPECT_EQ(color, outputColor);
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  canvas.rotate(50);
  canvas.drawRect(SkRect::MakeWH(255, 255), paint);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.HasGradient());

  SkColor gradientColors[2] = {SK_ColorWHITE, SK_ColorBLACK};
  SkPoint gradientPoints[2];
  gradientPoints[0].iset(0, 0);
  gradientPoints[1].iset(100, 0);
  skia::RefPtr<SkShader> shader(skia::AdoptRef(SkGradientShader::CreateLinear(
      gradientPoints, gradientColors, NULL, 2, SkShader::kClamp_TileMode)));
  SkPaint gradientPaint;
  gradientPaint.setShader(shader.get());

  canvas.drawRect(SkRect::MakeWH(255, 255), gradientPaint);

  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());
  EXPECT_TRUE(canvas.HasGradient());
}

TEST(AnalysisCanvasTest, FilterPaint) {
  skia::AnalysisCanvas canvas(255, 255,
                              skia::AnalysisCanvas::ANALYZE_SOLID_COLOR);
  SkPaint paint;

  paint.setImageFilter(SkOffsetImageFilter::Make(10, 10, nullptr));
  canvas.drawRect(SkRect::MakeWH(255, 255), paint);

  SkColor outputColor;
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
}

TEST(AnalysisCanvasTest, ClipPath) {
  skia::AnalysisCanvas canvas(255, 255,
                              skia::AnalysisCanvas::ANALYZE_SOLID_COLOR);

  // Skia will look for paths that are actually rects and treat
  // them as such. We add a divot to the following path to prevent
  // this optimization and truly test clipPath's behavior.
  SkPath path;
  path.moveTo(0, 0);
  path.lineTo(128, 50);
  path.lineTo(255, 0);
  path.lineTo(255, 255);
  path.lineTo(0, 255);

  SkColor outputColor;
  SolidColorFill(canvas);
  canvas.clipPath(path);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  canvas.save();
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  canvas.clipPath(path);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  canvas.restore();
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  SolidColorFill(canvas);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
}

TEST(AnalysisCanvasTest, SaveLayerWithXfermode) {
  skia::AnalysisCanvas canvas(255, 255,
                              skia::AnalysisCanvas::ANALYZE_SOLID_COLOR);
  SkRect bounds = SkRect::MakeWH(255, 255);
  SkColor outputColor;

  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  SkPaint paint;

  // Note: nothing is draw to the the save layer, but solid color
  // and transparency are handled conservatively in case the layer's
  // SkPaint draws something. For example, there could be an
  // SkPictureImageFilter. If someday analysis_canvas starts doing
  // a deeper analysis of the SkPaint, this test may need to be
  // redesigned.
  TransparentFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  paint.setXfermodeMode(SkXfermode::kSrc_Mode);
  canvas.saveLayer(&bounds, &paint);
  canvas.restore();
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  TransparentFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);
  canvas.saveLayer(&bounds, &paint);
  canvas.restore();
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  // Layer with dst xfermode is a no-op, so this is the only case
  // where solid color is unaffected by the layer.
  TransparentFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
  paint.setXfermodeMode(SkXfermode::kDst_Mode);
  canvas.saveLayer(&bounds, &paint);
  canvas.restore();
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
}

TEST(AnalysisCanvasTest, SaveLayerRestore) {
  skia::AnalysisCanvas canvas(255, 255,
                              skia::AnalysisCanvas::ANALYZE_SOLID_COLOR);

  SkColor outputColor;
  SolidColorFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));

  SkRect bounds = SkRect::MakeWH(255, 255);
  SkPaint paint;
  paint.setColor(SkColorSetARGB(255, 255, 255, 255));
  paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);

  // This should force non-transparency
  canvas.saveLayer(&bounds, &paint);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);

  TransparentFill(canvas);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  SolidColorFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);

  paint.setXfermodeMode(SkXfermode::kDst_Mode);

  // This should force non-solid color
  canvas.saveLayer(&bounds, &paint);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  TransparentFill(canvas);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  SolidColorFill(canvas);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  canvas.restore();
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  TransparentFill(canvas);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  SolidColorFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);

  canvas.restore();
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);

  TransparentFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_EQ(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);

  SolidColorFill(canvas);
  EXPECT_TRUE(canvas.GetColorIfSolid(&outputColor));
  EXPECT_NE(static_cast<SkColor>(SK_ColorTRANSPARENT), outputColor);
}

TEST(AnalysisCanvasTest, EarlyOutNotSolid) {
  SkRTreeFactory factory;
  SkPictureRecorder recorder;

  // Create a picture with 3 commands, last of which is non-solid.
  sk_sp<SkCanvas> record_canvas = sk_ref_sp(recorder.beginRecording(256, 256, &factory));

  std::string text = "text";
  SkPoint point = SkPoint::Make(SkIntToScalar(25), SkIntToScalar(25));

  SkPaint paint;
  paint.setColor(SkColorSetARGB(255, 255, 255, 255));
  paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);

  record_canvas->drawRect(SkRect::MakeWH(256, 256), paint);
  record_canvas->drawRect(SkRect::MakeWH(256, 256), paint);
  record_canvas->drawText(
      text.c_str(), text.length(), point.fX, point.fY, paint);

  sk_sp<SkPicture> picture = recorder.finishRecordingAsPicture();

  // Draw the picture into the analysis canvas, using the canvas as a callback
  // as well.
  skia::AnalysisCanvas canvas(256, 256,
                              skia::AnalysisCanvas::ANALYZE_SOLID_COLOR);
  picture->playback(&canvas, &canvas);

  // Ensure that canvas is not solid.
  SkColor output_color;
  EXPECT_FALSE(canvas.GetColorIfSolid(&output_color));

  // Verify that we aborted drawing.
  EXPECT_TRUE(canvas.abort());
}

TEST(AnalysisCanvasTest, EarlyOutGradient) {
  SkRTreeFactory factory;
  SkPictureRecorder recorder;

  // Create a picture with 3 commands, last of which is gradient draw.
  skia::RefPtr<SkCanvas> record_canvas =
      skia::SharePtr(recorder.beginRecording(256, 256, &factory));

  std::string text = "text";
  SkPoint point = SkPoint::Make(SkIntToScalar(25), SkIntToScalar(25));

  SkPaint paint;
  paint.setColor(SkColorSetARGB(255, 255, 255, 255));
  paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);

  SkColor gradientColors[2] = {SK_ColorWHITE, SK_ColorBLACK};
  SkPoint gradientPoints[2];
  gradientPoints[0].iset(0, 0);
  gradientPoints[1].iset(100, 0);
  skia::RefPtr<SkShader> shader(skia::AdoptRef(SkGradientShader::CreateLinear(
      gradientPoints, gradientColors, NULL, 2, SkShader::kClamp_TileMode)));
  SkPaint gradientPaint;
  gradientPaint.setShader(shader.get());

  record_canvas->drawRect(SkRect::MakeWH(256, 256), paint);
  record_canvas->drawText(text.c_str(), text.length(), point.fX, point.fY,
                          paint);
  record_canvas->drawRect(SkRect::MakeWH(256, 256), gradientPaint);

  skia::RefPtr<SkPicture> picture = skia::AdoptRef(recorder.endRecording());

  // Draw the picture into the analysis canvas, using the canvas as a callback
  // as well.
  int flags = skia::AnalysisCanvas::ANALYZE_SOLID_COLOR |
              skia::AnalysisCanvas::ANALYZE_LOSSLESS_BITMAP |
              skia::AnalysisCanvas::ANALYZE_GRADIENT;
  skia::AnalysisCanvas canvas(256, 256, flags);
  picture->playback(&canvas, &canvas);

  // Ensure that canvas has gradient.
  SkColor output_color;
  EXPECT_TRUE(canvas.HasGradient());
  EXPECT_FALSE(canvas.GetColorIfSolid(&output_color));
  EXPECT_FALSE(canvas.HasLargeLosslessBitmap());

  // Verify that we aborted drawing.
  EXPECT_TRUE(canvas.abort());
}

TEST(AnalysisCanvasTest, EarlyOutBitmap) {
  SkRTreeFactory factory;
  SkPictureRecorder recorder;

  // Create a picture with 3 commands, last of which is bitmap.
  skia::RefPtr<SkCanvas> record_canvas =
      skia::SharePtr(recorder.beginRecording(256, 256, &factory));

  std::string text = "text";
  SkPoint point = SkPoint::Make(SkIntToScalar(25), SkIntToScalar(25));

  SkPaint paint;
  paint.setColor(SkColorSetARGB(255, 255, 255, 255));
  paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);

  SkBitmap losslessLargeBitmap;
  losslessLargeBitmap.allocN32Pixels(255, 255);
  losslessLargeBitmap.setIsDecodedWithLosslessCodec(true);

  record_canvas->drawRect(SkRect::MakeWH(256, 256), paint);
  record_canvas->drawText(text.c_str(), text.length(), point.fX, point.fY,
                          paint);
  record_canvas->drawBitmap(losslessLargeBitmap, 0, 0);

  skia::RefPtr<SkPicture> picture = skia::AdoptRef(recorder.endRecording());

  // Draw the picture into the analysis canvas, using the canvas as a callback
  // as well.
  int flags = skia::AnalysisCanvas::ANALYZE_SOLID_COLOR |
              skia::AnalysisCanvas::ANALYZE_LOSSLESS_BITMAP |
              skia::AnalysisCanvas::ANALYZE_GRADIENT;
  skia::AnalysisCanvas canvas(256, 256, flags);
  picture->playback(&canvas, &canvas);

  // Ensure that canvas has gradient.
  SkColor output_color;
  EXPECT_TRUE(canvas.HasLargeLosslessBitmap());
  EXPECT_FALSE(canvas.GetColorIfSolid(&output_color));
  EXPECT_FALSE(canvas.HasGradient());

  // Verify that we aborted drawing.
  EXPECT_TRUE(canvas.abort());
}

TEST(AnalysisCanvasTest, ClipComplexRegion) {
  skia::AnalysisCanvas canvas(255, 255,
                              skia::AnalysisCanvas::ANALYZE_SOLID_COLOR);

  SkPath path;
  path.moveTo(0, 0);
  path.lineTo(128, 50);
  path.lineTo(255, 0);
  path.lineTo(255, 255);
  path.lineTo(0, 255);
  SkIRect pathBounds = path.getBounds().round();
  SkRegion region;
  region.setPath(path, SkRegion(pathBounds));

  SkColor outputColor;
  SolidColorFill(canvas);
  canvas.clipRegion(region);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  canvas.save();
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  canvas.clipRegion(region);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  canvas.restore();
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));

  SolidColorFill(canvas);
  EXPECT_FALSE(canvas.GetColorIfSolid(&outputColor));
}

}  // namespace skia
