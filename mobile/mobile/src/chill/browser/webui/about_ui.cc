// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/webui/about_ui.h"

#include <stdlib.h>  // for malloc/free

#include "base/memory/ref_counted_memory.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "grit/wam_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "third_party/lzma_sdk/LzmaLib.h"

#include "chill/common/opera_url_constants.h"

namespace opera {

std::string AboutUISource::GetSource() const {
  return kOperaAboutHost;
}

std::string AboutUISource::GetMimeType(const std::string& path) const {
  if (path == "opera_icon_red.png") {
    return "image/png";
  } else {
    return "text/html";
  }
}

void AboutUISource::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  if (path == "opera_icon_red.png") {
    base::RefCountedMemory* logo =
        ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
        IDR_ABOUT_ICON);
    callback.Run(logo);
  } else {
    static const int LZMA_PROPS_OFFSET = 0;
    static const int LZMA_DEST_SIZE_OFFSET = LZMA_PROPS_SIZE;
    static const int LZMA_DATA_OFFSET = LZMA_DEST_SIZE_OFFSET + 8;

    base::StringPiece resource =
        ResourceBundle::GetSharedInstance().GetRawDataResource(
            IDR_ABOUT_HTML);
    const unsigned char* compressed_data =
        (const unsigned char*)resource.data();
    size_t compressed_size = resource.size();

    size_t uncompressed_size = 0;
    for (int i = 0; i < 4; i++) {
      unsigned char a = *(compressed_data + LZMA_PROPS_SIZE + i);
      uncompressed_size += a << (i * 8);
    }
    unsigned char *dest = (unsigned char*)malloc(uncompressed_size);
    if (dest) {
      int ret = LzmaUncompress(dest, &uncompressed_size,
                           &compressed_data[LZMA_DATA_OFFSET],
                           &compressed_size,
                           &compressed_data[LZMA_PROPS_OFFSET],
                           LZMA_PROPS_SIZE);
      if (ret == 0) {
        std::string html_copy(reinterpret_cast<const char*>(dest));
        free(dest);
        callback.Run(base::RefCountedString::TakeString(&html_copy));
      } else {
        free(dest);
        callback.Run(NULL);
      }
    } else {
      callback.Run(NULL);
    }
  }
}

AboutUI::AboutUI(content::WebUI* web_ui, const std::string&)
    : WebUIController(web_ui) {
  content::URLDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
      new AboutUISource());
}

}  // namespace opera
