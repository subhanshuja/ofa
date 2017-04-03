// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/webui/net_internals/http_cache_view.js
// @last-synchronized b7dc66d590d1225b23736e869eb96b94add4ab2c

/**
 * This view displays information on the HTTP cache.
 */
var HttpCacheView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   *  @constructor
   */
  function HttpCacheView() {
    assertFirstConstructorCall(HttpCacheView);

    // Call superclass's constructor.
    superClass.call(this, HttpCacheView.MAIN_BOX_ID);

    this.statsDiv_ = $(HttpCacheView.STATS_DIV_ID);

    // Register to receive http cache info.
    g_browser.addHttpCacheInfoObserver(this, true);
  }

  HttpCacheView.TAB_ID = 'tab-handle-http-cache';
  HttpCacheView.TAB_NAME = 'Cache';
  HttpCacheView.TAB_HASH = '#httpCache';

  // IDs for special HTML elements in http_cache_view.html
  HttpCacheView.MAIN_BOX_ID = 'http-cache-view-tab-content';
  HttpCacheView.STATS_DIV_ID = 'http-cache-view-cache-stats';

  cr.addSingletonGetter(HttpCacheView);

  HttpCacheView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    onLoadLogFinish: function(data) {
      return this.onHttpCacheInfoChanged(data.httpCacheInfo);
    },

    onHttpCacheInfoChanged: function(info) {
      this.statsDiv_.innerHTML = '';

      if (!info)
        return false;

      // Print the statistics.
      var statsUl = addNode(this.statsDiv_, 'ul');
      for (var statName in info.stats) {
        var li = addNode(statsUl, 'li');
        addTextNode(li, statName + ': ' + info.stats[statName]);
      }
      return true;
    }
  };

  return HttpCacheView;
})();
