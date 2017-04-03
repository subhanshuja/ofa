/* -*- Mode: javascript; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2011-2015 Opera Software ASA.  All rights reserved.
 *
 * This file is an original work developed by Opera Software ASA
 */

/*
** Description: This program reads a binary (unencrypted, uncompressed) OBML
** data stream (using XMLHttpRequest) and converts it to a DOM representation
** that can be rendered by a standard browser (e.g. Opera Mobile).
**
** Tested in Opera, Chrome, Firefox and IE (some browsers don't support WebP).
*/

cr.define('obml', function() {
  'use strict';


  //--------------------------------------------------------------------------
  // OBML parser class
  //--------------------------------------------------------------------------

  function COBMLParser(path) {

    //--------------------------------------------------------------------------
    // OBML document raw data
    //--------------------------------------------------------------------------

    var mData;
    var mDataSize = 0, mHeaderSize = 0;
    var mPos = 0;


    //--------------------------------------------------------------------------
    // Parsed OBML data
    //--------------------------------------------------------------------------

    var mVersion, mDocWidth, mDocHeight, mWidth, mHeight, mTitle, mIconOffset,
      mBaseURL, mURL, mImageOffset0 = 0, mLinkTree = null;


    //--------------------------------------------------------------------------
    // Document state
    //--------------------------------------------------------------------------

    var mVirtualWidth = 0, mVirtualHeight = 0, mFirstFocus = 0, mLastFocus = 0,
      mThumbnail = 0, nx = 0, ny = 0, nw = 0, nh = 0, rx = 0, ry = 0,
      mBannerHeight = 0;


    //--------------------------------------------------------------------------
    // DOM object handling
    //--------------------------------------------------------------------------

    var mRoot;


    //--------------------------------------------------------------------------
    // Misc. helpers
    //--------------------------------------------------------------------------

    var logMsg = function (context, msg) {
      console.log(context + ": " + msg);
    };

    var createBanner = function (msg) {
      var bannerElement = document.createElement("div");
      bannerElement.className = "banner";
      bannerElement.style.width = mWidth + "px"
      bannerElement.innerHTML =
        '<input type="checkbox" id="banner-msg-toggle">' +
        '<label for="banner-msg-toggle" class="content">' +
        '  <h1>' + msg + '</h1>' +
        '  Tap to close this message, or visit the <a href="' + mURL + '">original page</a>.' +
        '</label>';

      return bannerElement;
    }


    //--------------------------------------------------------------------------
    // Data reading functions
    //--------------------------------------------------------------------------

    var readByte = function () {
      return mData[mPos++] & 0xff;
    };

    var readShort = function () {
      var p = mPos;
      var x = ((mData[p] & 0xff) << 8) +
        (mData[p + 1] & 0xff);
      mPos += 2;
      return x;
    };

    var readSignedShort = function () {
      var x = readShort();
      return x <= 32767 ? x : x - 65536;
    };

    var readNumber = function () {
      var p = mPos;
      var x = ((mData[p] & 0xff) << 16) +
        ((mData[p + 1] & 0xff) << 8) +
        (mData[p + 2] & 0xff);
      mPos += 3;
      return x;
    };

    var readSignedNumber = function () {
      var x = readNumber();
      return x <= 8388607 ? x : x - 16777216;
    };

    var readInt = function () {
      var p = mPos;
      var x = ((mData[p] & 0xff) << 24) +
        ((mData[p + 1] & 0xff) << 16) +
        ((mData[p + 2] & 0xff) << 8) +
        (mData[p + 3] & 0xff);
      mPos += 4;
      return x;
    };

    var readString = function () {
      var strLen = readShort();
      var p = mPos;
      var str = "", c;
      for (var i = 0; i < strLen;) {
        c = mData[p + i];
        i++;
        if ((c > 191) && (c < 224)) {
          c = ((c & 31) << 6) | (mData[p + i] & 63);
          i++;
        }
        else if (c >= 128) {
          c = ((c & 15) << 12) | ((mData[p + i] & 63) << 6) | (mData[p + i + 1] & 63);
          i += 2;
        }
        str += String.fromCharCode(c);
      }
      mPos += strLen;
      return str;
    };

    var readURL = function () {
      var s = readString();
      if (s.length > 0 && s.charCodeAt(0) == 0) {
        return mBaseURL + s.substring(1, s.length);
      }
      return s;
    };

    var readArea = function () {
      nx = readShort();
      ny = readNumber();
      nw = readShort();
      nh = readNumber();
    };

    var readRelativeArea = function () {
      nx = readSignedShort();
      ny = readSignedNumber();
      nw = readShort();
      nh = readNumber();
    };


    //--------------------------------------------------------------------------
    // Image decoding
    //--------------------------------------------------------------------------

    var decodeCImage = function (offset, len) {
      // Decode the C-image data to a canvas
      var w = mData[offset+1];
      var h = mData[offset+2];
      var canvas = document.createElement("canvas");
      canvas.width = w;
      canvas.height = h;
      var ctx = canvas.getContext("2d");

      var packed = "";
      for (var i = 0; i < len-3; ++i) {
        packed += String.fromCharCode(mData[offset+3+i]);
      }
      var unpacked = JSInflate.inflate(packed);

      var data = ctx.createImageData(w, h);
      for (var y = 0; y < h; ++ y) {
        for (var x = 0; x < w; ++ x) {
          var idx = (y*w + x) * 2;
          var c = ((unpacked.charCodeAt(idx) & 255) << 8) |
            (unpacked.charCodeAt(idx+1) & 255);
          idx *= 2;
          data.data[idx] = ((c >> 8) & 0x0f) * 0x11;
          data.data[idx+1] = ((c >> 4) & 0x0f) * 0x11;
          data.data[idx+2] = (c & 0x0f) * 0x11;
          data.data[idx+3] = ((c >> 12) & 0x0f) * 0x11;
        }
      }
      ctx.putImageData(data, 0, 0);

      return canvas.toDataURL("image/png");
    };

    var decodeImage = function (offset) {
      // Get image data length & image start offset
      var oldPos = mPos;
      mPos = offset;
      var imgLen = readShort();
      offset = mPos;
      mPos = oldPos;

      // Detect image type
      var mimeType;
      if (imgLen >= 23 && mData[offset] == 137) {
        // PNG
        mimeType = "image/png";
      }
      else if (imgLen >= 2 && mData[offset] == 255 && mData[offset + 1] == 252) {
        // JPEG
        mimeType = "image/jpeg";
      }
      else if (imgLen >= 20 &&
          mData[offset]    == 82 && mData[offset+1]  == 73 &&
          mData[offset+2]  == 70 && mData[offset+3]  == 70 &&
          mData[offset+8]  == 87 && mData[offset+9]  == 69 &&
          mData[offset+10] == 66 && mData[offset+11] == 80) {
        // WebP
        mimeType = "image/webp";
      }
      else if (imgLen >= 3 && mData[offset] == 67) {
        // C-image
        return decodeCImage(offset, imgLen);
      }
      else {
        // logMsg("decodeImage", "Unknown image type: " + mData[offset] + "," + mData[offset+1] + "," + mData[offset+2] + "," + mData[offset+3] + "," + mData[offset+4]);
        return "";
      }

      // Use the raw data from the OBML data & turn it into a data URI
      var binStr = "";
      for (var i = 0; i < imgLen; ++i) {
        binStr += String.fromCharCode(mData[offset + i]);
      }
      return "data:" + mimeType + ";base64," + window.btoa(binStr);
    };


    //--------------------------------------------------------------------------
    // Paint functions
    //--------------------------------------------------------------------------

    var color2rgba = function (color) {
      return "rgba(" +
        ((color >> 16) & 255) + "," +
        ((color >> 8) & 255) + "," +
        (color & 255) + "," +
        (((color >> 24) & 255) / 255) +
        ")";
    };

    var fillRectAlpha = function (x, y, w, h, color) {
      var e = document.createElement('div');
      e.style.backgroundColor = color2rgba(color);
      e.style.left = Math.round(x) + "px";
      e.style.top = Math.round(y) + "px";
      e.style.width = Math.round(w) + "px";
      e.style.height = Math.round(h) + "px";
      mRoot.appendChild(e);
    };

    var getFontClass = function (font) {
      return "f" + font;
    };

    var paintTextNode = function (x, y, nodePos, font,
        color, unscaledWidth, unscaledHeight,
        text, rtl) {
      var e;
      var url = getLinkURL(x, y, unscaledWidth, unscaledHeight);
      if (url) {
        e = document.createElement('a');
        e.href = url;
      }
      else {
        e = document.createElement('p');
      }
      e.style.color = color2rgba(color);
      e.className = getFontClass(font);
      e.style.left = Math.round(x) + "px";
      e.style.top = Math.round(y) + "px";
      e.style.width = Math.round(unscaledWidth) + "px";
      e.style.height = Math.round(unscaledHeight) + "px";
      if (rtl) {
        e.style.direction = "rtl";
      }
      e.innerHTML = text;
      mRoot.appendChild(e);
    };

    var paintImageNode = function (x, y, xoffset, yoffset,
        unscaledWidth, unscaledHeight,
        checksum, dataOffset, color,
        scaledWidth, scaledHeight) {
      var w = scaledWidth > 0 ? scaledWidth : unscaledWidth;
      var h = scaledHeight > 0 ? scaledHeight : unscaledHeight;
      var dataURI = decodeImage(dataOffset + mImageOffset0);
      if (dataURI.length > 0) {
        var e;
        var url = getLinkURL(x, y, w, h);
        if (url) {
          e = document.createElement('a');
          e.href = url;
        }
        else {
          e = document.createElement('div');
        }
        e.style.backgroundImage = "url('" + dataURI + "')";
        e.style.backgroundSize = Math.round(w) + "px " + Math.round(h) + "px";
        e.style.left = Math.round(x) + "px";
        e.style.top = Math.round(y) + "px";
        e.style.width = Math.round(w) + "px";
        e.style.height = Math.round(h) + "px";
        mRoot.appendChild(e);
      }
      else {
        fillRectAlpha(x, y, unscaledWidth, unscaledHeight, color);
      }
    };

    var paintInputNode = function (color, unscaledHeight) {
      var iType = readByte();
      var flags = readByte();

      var name = readString();
      var value = readString();
      var extra = readNumber();
      readShort();            // font if applicable

      // FIXME: Draw something here...
    };


    //--------------------------------------------------------------------------
    // OBML parsing functions header
    //--------------------------------------------------------------------------

    var skipMetas = function() {
      for(var nMetas = readByte(); nMetas > 0; nMetas--) {
        mPos++; // type
        var len = readShort();  // meta
        mPos += len;
      }
    }

    var skipTag = function (type) {
      var len;
      switch (type)
      {
        case 84: // 'T' text
          mPos += 6;           // COLOR FONT
          skipMetas();
          len = readShort();
          mPos += len;
          break;
        case 73: // 'I' image
          mPos += 7;
          skipMetas();
          break;
        case 66: // 'B' box
          mPos += 4;
          break;
        case 70: // 'F' input
          mPos += 6;
          len = readShort();
          mPos += len;
          len = readShort();
          mPos += len + 5;
          break;
        case 83: // 'S' skip
          len = readNumber();
          mPos += len;
          break;
        case 76: // 'L' first focus
          mPos += 9;
          break;
        case 77: // 'M' metadata
          mPos++;
          len = readNumber();
          mPos += len;
          break;
        default:
          logMsg("skipTag", "Unknown node type: "+type+" "+String.fromCharCode(type)+" at offset "+mPos);
      }
    };

    var readHeader = function () {
      mPos = 0;
      var mVersion = readByte(); // version


      mDocWidth = readShort();
      mDocHeight = readNumber();
      mWidth = Math.max(mDocWidth, mVirtualWidth);
      mHeight = Math.max(mDocHeight, mVirtualHeight);
      readShort(); // maxAge
      mTitle = readString();

      var iconLength = readShort();
      if (iconLength > 0) {
        mIconOffset = mPos - 2;
        mPos += iconLength;
      }
      else {
        mIconOffset = 0;
      }
      mBaseURL = readString();
      mURL = readURL();

      readByte(); // flags
      readShort(); // documentId
      mImageOffset0 = readNumber();

      mHeaderSize = mPos;
    };

    var buildLinkTreeNode = function (links, start, stop) {
      // Leaf node?
      if (start == stop) {
        return {
          x1: links[start].x1,
          y1: links[start].y1,
          x2: links[start].x2,
          y2: links[start].y2,
          url: links[start].url
        };
      }

      // Calculate bounding area
      var x1 = links[start].x1,
      y1 = links[start].y1,
        x2 = links[start].x2,
        y2 = links[start].y2;
      for (var i = start + 1; i <= stop; ++i) {
        if (links[i].x1 < x1) x1 = links[i].x1;
        if (links[i].y1 < y1) y1 = links[i].y1;
        if (links[i].x2 > x2) x2 = links[i].x2;
        if (links[i].y2 > y2) y2 = links[i].y2;
      }

      // Split along X or Y?
      var w = x2 - x1, h = y2 - y1;
      var storeIdx = start, mid, midFun, tmp;
      if (w > 2 * h) {
        mid = x1 + x2;
        midFun = function (link) { return link.x1 + link.x2; };
      }
      else {
        mid = y1 + y2;
        midFun = function (link) { return link.y1 + link.y2; };
      }
      for (var i = start; i <= stop; ++i) {
        if (midFun(links[i]) < mid) {
          tmp = links[storeIdx];
          links[storeIdx] = links[i];
          links[i] = tmp;
          storeIdx++;
        }
      }

      // Prevent degenerate cases
      if ((storeIdx == start) || (storeIdx > stop)) {
        storeIdx = (start + 1 + stop) >> 1;
      }

      // Create a tree node with children
      var childA = buildLinkTreeNode(links, start, storeIdx - 1),
      childB = buildLinkTreeNode(links, storeIdx, stop);
      return {
        x1: x1,
        y1: y1,
        x2: x2,
        y2: y2,
        a: childA,
        b: childB
      };
    };

    var getLinkURLFromTree = function (node, x, y) {
      if (!node || y < node.y1 || y > node.y2 ||
          x < node.x1 || x > node.x2) {
        return null;
      }
      if (node.url) {
        return node.url;
      }
      var url = getLinkURLFromTree(node.a, x, y);
      if (!url) {
        url = getLinkURLFromTree(node.b, x, y);
      }
      return url;
    };

    var getLinkURL = function (x, y, w, h) {
      // Check if the middle of the area is inside any of the link areas
      return getLinkURLFromTree(mLinkTree, x + (w >> 1), y + (h >> 1));
    };

    var readLinkInfo = function (start, stop) {
      var firstFocus = 0, lastFocus = 0, type;

      mLinkTree = null;

      // Get first/last focus data offsets
      for (mPos = start; mPos < stop; start = mPos)
      {
        type = readByte();
        switch (type) {
          case 76: // 'L' first focus
            readNumber(); // skip initial focus
            firstFocus = readNumber();
            lastFocus  = readNumber();
            break;
          case 83: // 'S' skip
          case 77: // 'M' metas
            skipTag(type);
            break;
          default:
            readRelativeArea();
            skipTag(type);
            break;
        }
      }
      if (lastFocus == 0) {
        return;
      }

      var linkURL, len, nareas, savedPos, e, goodURL, area, linkAreaArray = [];

      // Parse links into an array
      for (mPos = firstFocus; mPos <= lastFocus;) {
        type = readByte();
        nareas = readByte();

        // Read link URL (after the areas)
        linkURL = null;
        if (type == 76) { // 'L'
          savedPos = mPos;
          mPos += nareas * 10;  // Each area is 10 bytes
          linkURL = readURL();
          mPos = savedPos;
        }

        // Bogus link? Skip funny URLs starting with "b:"
        goodURL = linkURL && linkURL.substring(0,2) != "b:";

        // Read all areas
        for (; nareas > 0; nareas--) {
          readArea();
          if (goodURL) {
            area = { x1: nx, y1: ny, x2: nx+nw, y2: ny+nh, url: linkURL };
            linkAreaArray.push(area);
          }
        }

        // Skip to the end of this FOCUSDATA
        len = readShort(); // target
        mPos += len;
        skipMetas();
      }

      // Build a binary link area tree
      if (linkAreaArray.length > 0) {
        var start = 0;
        var stop = linkAreaArray.length - 1;
        mLinkTree = buildLinkTreeNode(linkAreaArray, start, stop);
      }
    };

    var buildDOM = function (start, stop) {
      var len, absx = 0, absy = 0;
      mPos = start;

      while (mPos < stop) {
        var nodePos = mPos;
        var type = readByte();

        if (type == 83 || type == 76 || type == 77) { // 'S' 'L' 'M'
          skipTag(type);
          continue;
        }

        readRelativeArea();
        // read relative coords, so convert to absolute
        nx += absx;
        ny += absy;
        // save unscaled coords
        absx = nx;
        absy = ny;

        var unscaledWidth = nw;
        var unscaledHeight = nh;

        var color = readInt();

        switch (type)
        {
          case 84: // 'T' text
            var font = readShort();
            // skipMetas();
            var rtl = false;
            for (var nMetas = readByte(); nMetas > 0; nMetas--) {
              switch (readByte())
              {
                case 114: // 'r' TEXTMETADIRECTION
                  {
                    readShort(); // zero len
                    rtl = true;
                    break;
                  }
                default:
                  len = readShort();
                  mPos += len; // skip unknown metas
                  break;
              }
            }
            var text = readString();
            paintTextNode(absx, absy, nodePos, font,
                color, unscaledWidth, unscaledHeight,
                text, rtl);
            break;

          case 73: // 'I' image
            var xoffset = 0; // readByte();
            var yoffset = 0; // readByte();
            // int flags = readByte();
            var dataOffset = readSignedNumber();
            var checksum = 0;
            var scaledWidth = -1;
            var scaledHeight = -1;
            // Read image metadata
            for (var nMetas = readByte(); nMetas > 0; nMetas--) {
              switch (readByte())
              {
                case 99: // 'c' IMAGEMETACHECKSUM: this must always be supplied by the server!
                  {
                    len = readShort();
                    checksum = readInt();
                    mPos += len - 4; // assumes len >= 4
                    break;
                  }
                case 111: // 'o' IMAGEMETATILEOFFSET = "o" LEN XOFFSET YOFFSET
                  mPos += 2; // len
                  xoffset = readShort();
                  yoffset = readShort();
                  break;
                case 122: // 'z' IMAGEMETAZOOM = LEN SCALEW SCALEH
                  {
                    len = readShort();
                    scaledWidth = readShort();
                    scaledHeight = readShort();
                    break;
                  }
                default:
                  // skip unknown metas
                  len = readShort();
                  mPos += len;
                  break;
              }
            }

            if (nw > 0 && nh > 0) {
              paintImageNode(absx, absy, xoffset, yoffset,
                  unscaledWidth, unscaledHeight,
                  checksum, dataOffset, color,
                  scaledWidth, scaledHeight);
            }
            break;

          case 66: // 'B' box
            fillRectAlpha(nx, ny, nw, nh, color);
            break;

          case 70: // 'F' input
            paintInputNode(color, unscaledHeight);
            break;

          default:
            logMsg("buildDOM", "Unknown node type: "+type+" ("+String.fromCharCode(type)+")");
            break;
        }
      }
    };

    var updateFavIcon = function () {
      if (!mIconOffset) return;
      mPos = mIconOffset;
      var len = readShort();
      var icon = document.getElementById("favicon");
      if (icon) {
        var dataURI = decodeImage(mIconOffset);
        if (dataURI.length > 0) {
          var newIcon = icon.cloneNode(true);
          newIcon.setAttribute('href', dataURI);
          icon.parentNode.replaceChild(newIcon, icon);
        }
      }
    };


    //--------------------------------------------------------------------------
    // Process the entire OBML document
    //--------------------------------------------------------------------------

    var processData = function () {
      var conversionException = null;

      // Root element; all content will be added to this element
      mRoot = document.createElement("div");

      try {
        // Read document header
        readHeader();

        mRoot.style.width = mWidth + "px"

        // Read link areas from the document
        readLinkInfo(mHeaderSize, mDataSize);

        // Draw document
        buildDOM(mHeaderSize, mDataSize);
      } catch (exception) {
        document.body.appendChild(createBanner("Something went wrong!"));
        conversionException = exception;
      } finally {
        // Add document to body
        document.title = mTitle;
        updateFavIcon();
        document.body.appendChild(mRoot);

        chrome.send('conversionCompleted', [path]);
      }

      if (conversionException) {
        throw conversionException;
      }
    };


    //--------------------------------------------------------------------------
    // Raw data loading functions
    //--------------------------------------------------------------------------

    var loadDataFromBinaryString = function (str) {
      mData = new Uint8Array(str.length);
      for (var i = 0; i < str.length; ++i) {
        mData[i] = str.charCodeAt(i);
      }
      mDataSize = mData.length;
      processData();
    };

    var loadDataFromHostPath = function (path) {
      // Create an XHR object to load the OBML document
      var req = new XMLHttpRequest();
      req.onreadystatechange = function () {
        if (req.readyState == 4) {
          // Get the binary data & process it
          if (typeof ActiveXObject == "function") {
            // IE path for handling binary data
            var data = new VBArray(req.responseBody).toArray();
            for (var j = 0; j < data.length; ++j)
              data[j] = String.fromCharCode(data[j]);
            loadDataFromBinaryString(data.join(''));
            req.abort();
          } else {
            // Regular path for handling binary data
            loadDataFromBinaryString(req.responseText);
          }
        }
      };
      req.open("GET", path, true);
      if (req.overrideMimeType) {
        req.overrideMimeType('text/plain; charset=x-user-defined');
      }
      req.send(null);
    };


    //--------------------------------------------------------------------------
    // Constructor
    //--------------------------------------------------------------------------

    loadDataFromHostPath(path);
  }

  function convert(path) {
    // Remove any already performed conversion
    var rootDiv = document.querySelector('body div');
    if (rootDiv !== null) {
      rootDiv.parentNode.removeChild(rootDiv);
      rootDiv = null;
    }

    var obmlParser = new COBMLParser(path);
  }

  function contentLoaded() {
    chrome.send('contentLoaded');
  }

  return {
    contentLoaded: contentLoaded,
    convert: convert
  }
});

document.addEventListener('DOMContentLoaded', obml.contentLoaded);
