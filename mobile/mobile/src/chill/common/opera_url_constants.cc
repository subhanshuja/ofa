// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/common/opera_url_constants.h"

#include "content/public/common/url_constants.h"

#include "opera_common/grit/product_free_common_strings.h"

namespace opera {

const char kOperaUIScheme[] = "chrome";
const char kOperaAboutScheme[] = "about";

const char kOperaAboutHost[] = "about";
const char kOperaCobrandSettingsHost[] = "cobrand-settings";
const char kOperaDebugZeroRatingHost[] = "debug-zero-rating";
const char kOperaFlagsHost[] = "flags";
const char kOperaLogcatHost[] = "logcat";
const char kOperaNetInternalsHost[] = "net-internals";
const char kOperaOBMLMigrationHost[] = "saved-obml-migration";
const char kOperaVersionHost[] = "version";

const OperaHostUrl kOperaHostUrls[] = {
    {content::kChromeUIGpuHost, IDS_OPERA_HOST_TITLE_GPU},
    {content::kChromeUIServiceWorkerInternalsHost,
     IDS_OPERA_HOST_TITLE_SERVICE_WORKER_INTERNALS},
    {kOperaAboutHost, IDS_OPERA_HOST_TITLE_ABOUT},
    {kOperaFlagsHost, IDS_OPERA_HOST_TITLE_FLAGS},
    {kOperaNetInternalsHost, IDS_OPERA_HOST_TITLE_NET_INTERNALS},
    {kOperaVersionHost, IDS_ABOUT_VERSION_TITLE}};

const size_t kNumberOfOperaHostUrls = arraysize(kOperaHostUrls);

}  // namespace opera
