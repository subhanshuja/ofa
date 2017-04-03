// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_DOWNLOAD_DOWNLOAD_HISTORY_DATA_H_
#define COMMON_MIGRATION_DOWNLOAD_DOWNLOAD_HISTORY_DATA_H_

#include <ctime>
#include <stdint.h>
#include <vector>
#include <string>

namespace opera {
namespace common {
namespace migration {

const uint8_t MSB_VALUE = 0x80;

const uint8_t TAG_DOWNLOAD_DOWNLOAD = 0x41;  // Indicates next download.

const uint8_t TAG_DOWNLOAD_URL = 0x03;
const uint8_t TAG_DOWNLOAD_LAST_VISITED = 0x04;
// The url is a result of a form query.
const uint8_t TAG_DOWNLOAD_FORM_QUERY_RESULT = (0x0b | MSB_VALUE);

// The local time when the file was last loaded.
const uint8_t TAG_DOWNLOAD_LOADED_TIME = 0x05;
// The status of load: 2 - loaded, 4 - loading aborted, 5 - loading failed
const uint8_t TAG_DOWNLOAD_LOAD_STATUS = 0x07;
const uint8_t TAG_DOWNLOAD_CONTENT_SIZE = 0x08;
const uint8_t TAG_DOWNLOAD_MIME_TYPE = 0x09;
// The character set of the content
const uint8_t TAG_DOWNLOAD_CHARACTER_SET = 0x0a;
// The file is downloaded and stored on user's disk;
// it is not part of the disk cache dir.
const uint8_t TAG_DOWNLOAD_STORED_LOCALLY = (0x0c | MSB_VALUE);
const uint8_t TAG_DOWNLOAD_FILE_NAME = 0x0d;
// Always check if modified.
const uint8_t TAG_DOWNLOAD_CHECK_IF_MODIFIED = (0x0f | MSB_VALUE);
// Contains the HTTP protocol specific information.
const uint8_t TAG_DOWNLOAD_HTTP_RECORD = 0x10;
// This signifies that the url is stored in the cache index.
// It is an array fo bytes that represent the content of the URL.
const uint8_t TAG_DOWNLOAD_EMBEDDED_URLS = 0x50;
// Identifies the network type from which a given resource has been loaded:
// 0: localhost, 1: private, 2: public, 3: undetermined.
const uint8_t TAG_DOWNLOAD_NETWORK_TYPE = 0x52;
// Used to detect if the MIME type was initially empty.
const uint8_t TAG_DOWNLOAD_MIME_INITIALLY_EMPTY = 0x53;
const uint8_t TAG_DOWNLOAD_FTP_MODIFIED_DATE = 0x2b;  // FTP Modified Date Time.
// Never flush this url.
const uint8_t TAG_DOWNLOAD_NEVER_FLUSH = (0x2b | MSB_VALUE);
// Theese are associated files.
const uint8_t TAG_DOWNLOAD_ASSOCIATED_FILES = 0x37;

// Identifies the time when the loading of the last/previous segment of
// the downloaded file started.
const uint8_t TAG_DOWNLOAD_SEGMENT_START_TIME = 0x28;
// Identifies the time when the loading of the last/previous segment of
// the downloaded file was stopped.
const uint8_t TAG_DOWNLOAD_SEGMENT_STOP_TIME = 0x29;

/** The is the amount of bytes in the previous segment of the file being
    downloaded. If the time the loading ended is not known, this value will be
    assumed to be zero (0) and the download speed set to zero (unknown). */
const uint8_t TAG_DOWNLOAD_SEGMENT_AMOUNT_OF_BYTES = 0x2a;

// Enables the ability to resume the downloaded URL. 0: not resumable,
// 1: may not be resumable (or field not present), 2: Probably resumable.
const uint8_t TAG_DOWNLOAD_RESUME_ABILITY = 0x2c;

struct DownloadInfo {
  DownloadInfo();
  DownloadInfo(const DownloadInfo&);
  ~DownloadInfo();

  // Each entry corresponds to one tag. Those are listed above.

  std::string url_;
  std::time_t last_visited_;
  bool form_query_result_;

  std::time_t loaded_time_;
  enum LoadStatus {
    LOAD_STATUS_UNKNOWN = 0,
    LOAD_STATUS_LOADED = 2,
    LOAD_STATUS_ABORTED = 4,
    LOAD_STATUS_FAILED = 5
  } load_status_;
  uint64_t content_size_;
  std::string mime_type_;
  std::string character_set_;
  bool stored_locally_;
  std::string file_name_;
  bool check_if_modified_;
  std::vector<char> http_record_;
  std::vector<char> embedded_urls_;
  uint8_t network_type_;
  bool mime_initially_empty_;
  std::string ftp_modified_date_;
  bool never_flush_;
  uint32_t associated_files_;

  std::time_t segment_start_time_;
  std::time_t segment_stop_time_;
  uint32_t segment_amount_of_bytes_;
  uint32_t resume_ability_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_DOWNLOAD_DOWNLOAD_HISTORY_DATA_H_
