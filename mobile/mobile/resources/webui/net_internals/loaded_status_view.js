// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/webui/net_internals/loaded_status_view.js
// @last-synchronized b7dc66d590d1225b23736e869eb96b94add4ab2c

/**
 * The status view at the top of the page when viewing a loaded dump file.
 */
var LoadedStatusView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  function LoadedStatusView() {
    superClass.call(this, LoadedStatusView.MAIN_BOX_ID);
  }

  LoadedStatusView.MAIN_BOX_ID = 'loaded-status-view';
  LoadedStatusView.DUMP_FILE_NAME_ID = 'loaded-status-view-dump-file-name';

  LoadedStatusView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    setFileName: function(fileName) {
      $(LoadedStatusView.DUMP_FILE_NAME_ID).innerText = fileName;
    }
  };

  return LoadedStatusView;
})();
