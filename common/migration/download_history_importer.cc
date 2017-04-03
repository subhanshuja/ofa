// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/download_history_importer.h"

#include "base/logging.h"
#include "common/migration/tools/data_stream_reader.h"

namespace opera {
namespace common {
namespace migration {

DownloadHistoryImporter::DownloadHistoryImporter(
    scoped_refptr<DownloadHistoryReceiver> downloadReceiver)
    : download_receiver_(downloadReceiver) {
}

DownloadHistoryImporter::~DownloadHistoryImporter() {
}

bool DownloadHistoryImporter::IsSpecOk(DataStreamReader* input) {
  if (input->IsFailed())
    return false;

  const DataStreamReader::Spec& spec = input->GetSpec();
  return (spec.file_version == 4096 &&
          spec.app_version >= 0x00020000 &&
          spec.app_version < 0x00030000 &&
          spec.tag_length == 1 &&
          spec.len_length == 2);
}

bool DownloadHistoryImporter::Parse(std::istream* input) {
  DataStreamReader reader(input, true /* big endian */);
  if (!IsSpecOk(&reader)) {
    LOG(ERROR) << "Data stream has invalid spec, aborting import";
    return false;
  }

  DownloadInfo info;

  uint32_t tag = reader.ReadTag();
  reader.ReadSize();  // skip the size bytes
  const uint16_t unknown_tag_limit = 5;
  uint16_t unknown_tags = 0;

  if (reader.IsFailed()) {
    LOG(ERROR) << "Failed to read a tag or its data.";
    return false;
  }

  if (tag != TAG_DOWNLOAD_DOWNLOAD) {
    LOG(ERROR) << "Missing first new download entry indication tag.";
    return false;
  }

  while (true) {
    tag = reader.ReadTag();

    if (reader.IsFailed() || reader.IsEof()) {
      LOG(ERROR) << "Failed to read a tag or its data.";
      return false;
    }

    switch (tag) {
    case TAG_DOWNLOAD_DOWNLOAD:
      downloads_.push_back(info);
      info = DownloadInfo();
      reader.ReadSize();  // skip the size bytes
      break;
    case TAG_DOWNLOAD_URL:
      info.url_ = reader.ReadString();
      break;
    case TAG_DOWNLOAD_LAST_VISITED:
      info.last_visited_ = reader.ReadContentWithSize<int32_t>();
      break;
    case TAG_DOWNLOAD_FORM_QUERY_RESULT:
      info.form_query_result_ = true;
      break;
    case TAG_DOWNLOAD_LOADED_TIME:
      info.loaded_time_ = reader.ReadContentWithSize<int64_t>();
      break;
    case TAG_DOWNLOAD_LOAD_STATUS:
      info.load_status_ = static_cast<DownloadInfo::LoadStatus>(
                              reader.ReadContentWithSize<uint32_t>());
      break;
    case TAG_DOWNLOAD_CONTENT_SIZE:
      info.content_size_ = reader.ReadContentWithSize<uint64_t>();
      break;
    case TAG_DOWNLOAD_MIME_TYPE:
      info.mime_type_ = reader.ReadString();
      break;
    case TAG_DOWNLOAD_CHARACTER_SET:
      info.character_set_ = reader.ReadString();
      break;
    case TAG_DOWNLOAD_STORED_LOCALLY:
      info.stored_locally_ = true;
      break;
    case TAG_DOWNLOAD_FILE_NAME:
      info.file_name_ = reader.ReadString();
      break;
    case TAG_DOWNLOAD_CHECK_IF_MODIFIED:
      info.check_if_modified_ = true;
      break;
    case TAG_DOWNLOAD_HTTP_RECORD:
      info.http_record_ = reader.ReadVectorWithSize<char>();
      break;
    case TAG_DOWNLOAD_EMBEDDED_URLS:
      info.embedded_urls_ = reader.ReadVectorWithSize<char>();
      break;
    case TAG_DOWNLOAD_NETWORK_TYPE:
      info.network_type_ = uint8_t(reader.ReadContentWithSize<uint32_t>());
      break;
    case TAG_DOWNLOAD_MIME_INITIALLY_EMPTY:
      info.mime_initially_empty_ = true;
      break;
    case TAG_DOWNLOAD_FTP_MODIFIED_DATE:
      info.ftp_modified_date_ = reader.ReadString();
      break;
    case TAG_DOWNLOAD_NEVER_FLUSH:
      info.never_flush_ = true;
      break;
    case TAG_DOWNLOAD_ASSOCIATED_FILES:
      info.associated_files_ = reader.ReadContentWithSize<uint32_t>();
      break;
    case TAG_DOWNLOAD_SEGMENT_START_TIME:
      info.segment_start_time_ = reader.ReadContentWithSize<int32_t>();
      break;
    case TAG_DOWNLOAD_SEGMENT_STOP_TIME:
      info.segment_stop_time_ = reader.ReadContentWithSize<int32_t>();
      break;
    case TAG_DOWNLOAD_SEGMENT_AMOUNT_OF_BYTES:
      info.segment_amount_of_bytes_ = reader.ReadContentWithSize<uint32_t>();
      break;
    case TAG_DOWNLOAD_RESUME_ABILITY: {
      uint32_t resume_ability = reader.ReadContentWithSize<uint32_t>();
      if (resume_ability == 0)
        info.resume_ability_ = 0;
      else if (resume_ability == 2)
        info.resume_ability_ = 2;
      else
        info.resume_ability_ = 1;
      }
      break;
    default:
      if (tag != 0x16) {  // 0x16 is deliberately skipped.
        unknown_tags++;
        LOG(WARNING) << "Unknown tag (" << tag << "), skipped.";
      }
      if (!(tag & MSB_VALUE))
        reader.SkipRecord();

      if (unknown_tags > unknown_tag_limit) {
        LOG(ERROR) << "Too many unkown tags."
                   << " Either the input is corrupted or too much data is lost";
        return false;
      }
      break;
    }

    if (reader.IsFailed()) {
      LOG(ERROR) << "Failed to read a tag or its data.";
      return false;
    }

    if (reader.IsEof()) {
      downloads_.push_back(info);
      break;
    }
  }

  return true;
}

void DownloadHistoryImporter::Import(
    std::unique_ptr<std::istream> input,
    scoped_refptr<MigrationResultListener> listener) {
  bool result = Parse(input.get());

  if (result && download_receiver_) {
    download_receiver_->OnNewDownloads(downloads_, listener);
    return;
  }
  listener->OnImportFinished(opera::DOWNLOADS, result);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
