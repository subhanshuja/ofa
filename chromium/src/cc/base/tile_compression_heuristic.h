// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BASE_TILE_COMPRESSION_HEURISTIC_H_
#define CC_BASE_TILE_COMPRESSION_HEURISTIC_H_

namespace cc {

// Heuristic to use during tile analysis to determine if a tile is not suitable
// for compression.
enum TileCompressionHeuristicFlags {
  TILE_COMPRESSION_HEURISTIC_GRADIENT = 1 << 0,
  TILE_COMPRESSION_HEURISTIC_IMAGE = 1 << 1,
};

}  // namespace cc

#endif  // CC_BASE_TILE_COMPRESSION_HEURISTIC_H_
