// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/web_storage_importer.h"

#include <map>
#include <utility>

#include "base/base64.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "libxml/parser.h"

#include "common/migration/tools/bulk_file_reader.h"
#include "common/migration/tools/util.h"
#include "common/migration/web_storage_listener.h"

namespace opera {
namespace common {
namespace migration {

WebStorageImporter::WebStorageImporter(
    scoped_refptr<BulkFileReader> reader,
    scoped_refptr<WebStorageListener> listener,
    bool is_extension)
    : reader_(reader), listener_(listener), is_extension_(is_extension) {
}

WebStorageImporter::~WebStorageImporter() {
}

void WebStorageImporter::Import(
    std::unique_ptr<std::istream> input,
    scoped_refptr<MigrationResultListener> listener) {
  listener->OnImportFinished(opera::WEB_STORAGE,
                             ImportImplementation(input.get()));
}

bool WebStorageImporter::ImportImplementation(std::istream* input) {
  if (!reader_) {
    LOG(ERROR) << "File reader not set, cannot import Web Storage";
    return false;
  }

  if (!ReadInitialInput(input))
    return false;
  return ReadDataFiles();
}

bool WebStorageImporter::ReadInitialInput(std::istream* input) {
  std::string psindexContent((std::istreambuf_iterator<char>(*input)),
                              std::istreambuf_iterator<char>());
  xmlDocPtr doc = xmlReadMemory(psindexContent, "psindex.dat");
  if (!doc) {
    LOG(ERROR) << "Could not parse initial input as xml";
    return false;
  }
  XmlDocAnchor anchor(doc);
  xmlNode* root_element = xmlDocGetRootElement(doc);
  for (xmlNode* child = root_element->children; child; child = child->next) {
    std::string name = reinterpret_cast<const char*>(child->name);
    if (name == "section") {
      bool section_parsed = ParseSection(child);
      if (!section_parsed) {
        LOG(ERROR) << "Failure when parsing section in the index file";
        return false;
      }
    }
  }
  return true;
}

bool WebStorageImporter::ParseSection(xmlNode* node) {
  typedef std::map<std::string, std::string> KeyValueMap;
  KeyValueMap value_content_map;

  /* After the following loop, value_content_map should have entries like:
   * value_content_map["DataFile"] = "pstorage/00/15/00000000" <- '\' on win
   * value_content_map["Origin"] = "http://www.cnn.com"
   * value_content_map["Type"] = "localstorage"
   */
  for (xmlNode* child = node->children; child; child = child->next) {
    if (child->properties &&
        child->properties->children &&
        child->children) {
      std::string value_id = reinterpret_cast<const char*>(
            child->properties->children->content);
      std::string content = reinterpret_cast<const char*>(
            child->children->content);
      value_content_map[value_id] = content;
    }
  }

  KeyValueMap::iterator url_iterator = value_content_map.find("Origin");
  KeyValueMap::iterator file_iterator = value_content_map.find("DataFile");
  if (url_iterator == value_content_map.end() ||
      file_iterator == value_content_map.end()) {
    LOG(ERROR) << "Origin url or DataFile was not set in the section";
    return false;
  }

  /* Each extension has its own separate localstorage in folder, so we only
   * need to know which storage type values belong to.
   */
  KeyValueMap::iterator type_iterator = value_content_map.find("Type");
  if (is_extension_ && type_iterator == value_content_map.end())
      return false;
  files_to_parse_.push_back(
      std::make_pair(file_iterator->second,
          is_extension_ ? type_iterator->second : url_iterator->second));
  return true;
}

bool WebStorageImporter::ReadDataFiles() {
  for (FilesToParseVector::iterator it = files_to_parse_.begin();
       it != files_to_parse_.end();
       ++it) {
    if (!ParseStorageFileContent(*it))
      return false;
  }
  files_to_parse_.clear();
  return true;
}

bool WebStorageImporter::ParseStorageFileContent(
    const FileUrlPair &data) {
  base::FilePath path = base::FilePath().AppendASCII(data.first);
  const std::string& origin_url = data.second;
  std::string file_content = reader_->ReadFileContent(path);
  if (file_content.empty()) {
    LOG(ERROR) << "File " << data.first << " is empty or doesn't exist";
    return false;
  }

  xmlDocPtr doc = xmlReadMemory(file_content, origin_url.c_str());
  if (!doc) {
    LOG(ERROR) << "Could not parse '" << data.first <<"' as xml";
    return false;
  }
  XmlDocAnchor anchor(doc);
  xmlNode* root_element = xmlDocGetRootElement(doc);
  for (xmlNode* child = root_element->children; child; child = child->next) {
    if (std::string(reinterpret_cast<const char*>(child->name)) == "e") {
      if (!ParseElementNode(child, origin_url)) {
        LOG(ERROR) << "Could not parse element node";
        return false;
      }
    }
  }
  return true;
}

namespace {
base::string16 GetDecoded(xmlNode* elem) {
  if (!elem->children)
    return base::string16();
  std::string encoded =
      reinterpret_cast<const char*>(elem->children->content);
  std::string decoded;
  if (!base::Base64Decode(encoded, &decoded))
    return base::string16();
  // Cant find a working way to convert 16-bit string to utf16
  // Tried ASCIItoUTF16, UTF8toUTF16 from base/strings/utf_string_conversions.h
  // They all produce input like "au\000bu\000" instead of "ab"
  base::string16 decoded16;
  for (size_t i = 0; i < decoded.size(); i += 2) {
    uint16_t c = *reinterpret_cast<uint16_t*>(&decoded[i]);
    decoded16.push_back(c);
  }
  return decoded16;
}
}  // namespace anonymous

bool WebStorageImporter::ParseElementNode(xmlNode* element_node,
                                          const std::string& origin_url) {
  typedef std::pair<base::string16, base::string16> KeyValuePair;
  std::vector<KeyValuePair> pairs;
  for (xmlNode* child = element_node->children; child; child = child->next) {
    // Found key?
    if (std::string(reinterpret_cast<const char*>(child->name)) == "k") {
      base::string16 key = GetDecoded(child);
      base::string16 value;
      // Value follows?
      if (child->next &&
          std::string(reinterpret_cast<const char*>(
              child->next->name)) == "v")
        value = GetDecoded(child->next);
      // Maybe that was a "text" node with "\n" content, try the next node
      else if (child->next &&
          child->next->next &&
          std::string(reinterpret_cast<const char*>(
              child->next->next->name)) == "v")
        value = GetDecoded(child->next->next);
      pairs.push_back(std::make_pair(key, value));
    }
  }
  if (listener_)
    return listener_->OnUrlImported(origin_url, pairs);
  return true;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
