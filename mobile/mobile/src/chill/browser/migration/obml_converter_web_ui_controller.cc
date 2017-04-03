// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/migration/obml_converter_web_ui_controller.h"

#include "base/bind.h"
#include "base/files/memory_mapped_file.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/mhtml_generation_params.h"
#include "grit/wam_resources.h"

#include "chill/browser/migration/obml_font_info.h"

namespace {

// Finds the offset from the beginning of the file to when the HEADER section
// starts. Saved OBML files seem to have the following structure:
// [TRANSCODING_PROGRESS] ESTIMATED_COMPRESSED_LENGTH COMMANDSTREAM
//    CONTENTSTREAM
//
// An excerpt from the OBML format spec:
// ESTIMATED_COMPRESSED_LENGTH = INT24
// NUMBYTES = INT24
//
// COMMANDSTREAM = NUMBYTES COMMANDNODES
// CONTENTSTREAM = NUMBYTES HEADER NODES
//
// The JS converter expects its input to start at the HEADER node, which sits in
// CONTENTSTREAM. See doc/obml2d.txt in the mini server repository for more
// information.
size_t OffsetToOBMLHeader(const base::MemoryMappedFile& obml_file) {
  static const uint8_t kProgressNodeMark = 0xff;
  static const unsigned int kProgressNodeSize = 2;
  static const unsigned int kInt24ByteSize = 3;

  size_t offset = 0;

  // Skip past the TRANSCODING_PROGRESS nodes
  while (*(obml_file.data() + offset) == kProgressNodeMark)
    offset += kProgressNodeSize;

  offset += kInt24ByteSize;

  // Reads (and skips past) the INT24 prepending the COMMANDNODES
  uint32_t commandnodes_num_bytes = 0;
  commandnodes_num_bytes = 0;
  int bytes_to_read = kInt24ByteSize;
  while (bytes_to_read-- > 0) {
    commandnodes_num_bytes =
        (commandnodes_num_bytes << 8) | *(obml_file.data() + offset++);
  }

  offset += commandnodes_num_bytes;
  offset += kInt24ByteSize;

  return offset;
}

void ReadSavedFileOnFileThread(
    const base::FilePath& saved_file_path,
    const content::WebUIDataSource::GotDataCallback& callback) {
  base::MemoryMappedFile saved_file_data;
  saved_file_data.Initialize(saved_file_path);
  DCHECK(saved_file_data.IsValid());

  size_t header_offset = OffsetToOBMLHeader(saved_file_data);

  // Copy the contents of the file to the response memory, but start copying at
  // the beginning of the HEADER node.
  // The callback will be the owner of the data.
  base::RefCountedBytes* response =
      new base::RefCountedBytes(saved_file_data.data() + header_offset,
                                saved_file_data.length() - header_offset);

  // The callback is responsible for moving to the correct thread
  callback.Run(response);
}

}  // namespace

namespace opera {

class OBMLConverterWebUIController::MessageHandler
    : public content::WebUIMessageHandler {
 public:
  explicit MessageHandler(OBMLConverterWebUIController* parent)
      : parent_(parent) {}

  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "contentLoaded", base::Bind(&MessageHandler::HandleContentLoaded,
                                    base::Unretained(this)));
    web_ui()->RegisterMessageCallback(
        "conversionCompleted",
        base::Bind(&MessageHandler::HandleConversionCompletion,
                   base::Unretained(this)));
  }

 private:
  void HandleContentLoaded(const base::ListValue* args) {
    parent_->SetContentLoaded();
  }

  void HandleConversionCompletion(const base::ListValue* args) {
    std::string path;
    bool got_path_string = args->GetString(0, &path);
    DCHECK(got_path_string);
    parent_->GenerateMHTML(path);
  }

  OBMLConverterWebUIController* parent_;
};

OBMLConverterWebUIController* OBMLConverterWebUIController::FromWebUIController(
    content::WebUIController* web_ui_controller) {
  return static_cast<OBMLConverterWebUIController*>(web_ui_controller);
}

OBMLConverterWebUIController::OBMLConverterWebUIController(
    content::WebUI* web_ui,
    std::string source_name,
    const std::vector<OBMLFontInfo>* font_info,
    base::FilePath conversion_target_directory)
    : WebUIController(web_ui),
      WebContentsObserver(web_ui->GetWebContents()),
      font_info_(font_info),
      content_loaded_(false),
      allowed_restarts_(0),
      conversion_target_directory_(conversion_target_directory) {
  web_ui->AddMessageHandler(new MessageHandler(this));
  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                CreateDataSource(source_name));
}

void OBMLConverterWebUIController::SetContentLoaded() {
  DCHECK(!content_loaded_);
  content_loaded_ = true;

  if (!pending_conversions_.empty()) {
    ConversionRequest pending_conversion = pending_conversions_.front();
    pending_conversions_.pop();

    StartConversion(pending_conversion.first, pending_conversion.second);
  }
}

void OBMLConverterWebUIController::FinalizeConversion(
    const base::FilePath& mhtml_file,
    int64_t file_size) {
  active_conversion_->second.Run(true, mhtml_file);

  active_conversion_.reset();
  StartNextConversion();
}

void OBMLConverterWebUIController::ReloadWebContents() {
  web_contents()->GetController().Reload(false);
}

void OBMLConverterWebUIController::GenerateMHTML(const std::string& path) {
  DCHECK_EQ(path, active_conversion_->first.BaseName().value());

  base::FilePath mhtml_file = conversion_target_directory_.Append(path);
  mhtml_file = mhtml_file.AddExtension("mhtml");

  web_ui()->GetWebContents()->GenerateMHTML(content::MHTMLGenerationParams(mhtml_file),
      base::Bind(&OBMLConverterWebUIController::FinalizeConversion,
                 base::Unretained(this), mhtml_file));
}

void OBMLConverterWebUIController::RenderProcessGone(
    base::TerminationStatus status) {
  content_loaded_ = false;

  if (IsRunning()) {
    pending_conversions_.push(*active_conversion_.get());
    active_conversion_.reset();
  }

  if (allowed_restarts_ > 0) {
    // Always assume a terminated render process is either a crash or an OOM
    // kill. The RenderProcessHost is not fully set up to recreate the render
    // process until after multiple passes on the UI thread loop, and no other
    // call is ever made to notify this. So, always wait a little while before
    // attempting to reload the WebContents to give the RenderProcessHost some
    // time to settle down.
    static const int kWebContentsReloadDelayMs = 250;
    content::BrowserThread::PostNonNestableDelayedTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(&OBMLConverterWebUIController::ReloadWebContents,
                   base::Unretained(this)),
        base::TimeDelta::FromMilliseconds(kWebContentsReloadDelayMs));
    allowed_restarts_--;
  } else if (!pending_conversions_.empty()) {
    LOG(ERROR) << "Could not complete OBML conversion";
    while (!pending_conversions_.empty()) {
      pending_conversions_.front().second.Run(false, base::FilePath());
      pending_conversions_.pop();
    }
  }
}

bool OBMLConverterWebUIController::IsRunning() const {
  return !!active_conversion_;
}

void OBMLConverterWebUIController::ConvertSavedPage(
    const base::FilePath& saved_page_path,
    const ConversionCallback& callback) {
  allowed_restarts_++;

  if (!content_loaded_ || active_conversion_.get() != nullptr) {
    pending_conversions_.push(ConversionRequest(saved_page_path, callback));
    return;
  }

  StartConversion(saved_page_path, callback);
}

void OBMLConverterWebUIController::StartNextConversion() {
  if (pending_conversions_.empty())
    return;

  ConversionRequest pending_conversion = pending_conversions_.front();
  pending_conversions_.pop();
  StartConversion(pending_conversion.first, pending_conversion.second);
}

void OBMLConverterWebUIController::StartConversion(
    const base::FilePath& saved_page_path,
    const ConversionCallback& callback) {
  DCHECK(!active_conversion_.get());
  active_conversion_.reset(new ConversionRequest(saved_page_path, callback));

  base::StringValue path_basename(saved_page_path.BaseName().value());

  web_ui()->CallJavascriptFunctionUnsafe("obml.convert", path_basename);
}

bool OBMLConverterWebUIController::HandleWebUIRequest(
    const std::string& path,
    const content::WebUIDataSource::GotDataCallback& callback) {
  DCHECK(font_info_);

  if (path == "font_rules.css") {
    callback.Run(CreateFontRuleStylesheet());
    return true;
  }

  if (active_conversion_) {
    base::FilePath active_conversion_path = active_conversion_->first;
    if (path == active_conversion_path.BaseName().value()) {
      RespondWithSavedFile(active_conversion_path, callback);
      return true;
    }
  }

  return false;
}

content::WebUIDataSource* OBMLConverterWebUIController::CreateDataSource(
    const std::string& source_name) {
  content::WebUIDataSource* data_source =
      content::WebUIDataSource::Create(source_name);

  data_source->SetRequestFilter(
      base::Bind(&OBMLConverterWebUIController::HandleWebUIRequest,
                 base::Unretained(this)));

  data_source->AddResourcePath("obml.css", IDR_OBML_CONVERTER_CSS);
  data_source->AddResourcePath("obml.js", IDR_OBML_CONVERTER_JS);
  data_source->AddResourcePath("inflate.min.js", IDR_INFLATE_MIN_JS);
  data_source->SetDefaultResource(IDR_OBML_CONVERTER_HTML);

  return data_source;
}

scoped_refptr<base::RefCountedString>
OBMLConverterWebUIController::CreateFontRuleStylesheet() const {
  static const char* kFontRuleFormat = ".f%d { font: %s; }\n";
  static const char* kFontShorthand = "%dpx/%dpx %s";

  std::string response;
  for (auto it = font_info_->begin(); it != font_info_->end(); it++) {
    OBMLFontInfo font_info = *it;

    std::string font_family;
    switch (font_info.family) {
      case OBMLFontInfo::Family::SANS_SERIF:
        font_family = "sans-serif";
        break;
      case OBMLFontInfo::Family::MONOSPACE:
        font_family = "monospace";
        break;
      case OBMLFontInfo::Family::SERIF:
      default:
        font_family = "serif";
        break;
    }

    std::string shorthand_font =
        base::StringPrintf(kFontShorthand, font_info.size,
                           font_info.line_height, font_family.c_str());

    if (font_info.bold)
      shorthand_font = "bold " + shorthand_font;

    if (font_info.italic)
      shorthand_font = "italic " + shorthand_font;

    std::string font_rule = base::StringPrintf(
        kFontRuleFormat, font_info.obml_id, shorthand_font.c_str());

    response += font_rule;
  }

  return base::RefCountedString::TakeString(&response);
}

void OBMLConverterWebUIController::RespondWithSavedFile(
    const base::FilePath& file_path,
    const content::WebUIDataSource::GotDataCallback& callback) {
  content::BrowserThread::PostTask(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&ReadSavedFileOnFileThread, file_path, callback));
}

}  // namespace opera
