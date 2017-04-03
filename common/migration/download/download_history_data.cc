// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/download/download_history_data.h"

using opera::common::migration::DownloadInfo;

DownloadInfo::DownloadInfo()
  : last_visited_(0),
    form_query_result_(false),
    loaded_time_(0),
    load_status_(LOAD_STATUS_UNKNOWN),
    content_size_(0),
    stored_locally_(false),
    check_if_modified_(false),
    network_type_(3),
    mime_initially_empty_(false),
    never_flush_(false),
    associated_files_(0),
    segment_start_time_(0),
    segment_stop_time_(0),
    segment_amount_of_bytes_(0),
    resume_ability_(1) {
}

DownloadInfo::DownloadInfo(const DownloadInfo&) = default;

DownloadInfo::~DownloadInfo() {
}
