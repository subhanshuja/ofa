// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/webui/net_internals/halted_status_view.js
// @last-synchronized b7dc66d590d1225b23736e869eb96b94add4ab2c

/**
 * The status view at the top of the page after stopping capturing.
 */
var HaltedStatusView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  function HaltedStatusView() {
    superClass.call(this, HaltedStatusView.MAIN_BOX_ID);
  }

  HaltedStatusView.MAIN_BOX_ID = 'halted-status-view';

  HaltedStatusView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype
  };

  return HaltedStatusView;
})();
