// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/savedpages.h"

#include <stdint.h>

#include <fstream>
#include <istream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "base/strings/utf_string_conversions.h"
#include "base/third_party/icu/icu_utf.h"
#include "base/sys_byteorder.h"

#include "mobile/common/favorites/favorites_impl.h"
#include "mobile/common/favorites/savedpage.h"

using base::IsAsciiWhitespace;
using base::IsHexDigit;
using base::HexDigitToInt;

namespace {

// Time after which saved page metadata will be written to disk once it has been
// modified.
const int kWriterCommitIntervalSeconds = 10;

// File (in FavoritesImpl::path_) where saved page meta data is stored.
const char* kSavedPagesFile = "savedpages.db";

// Version string for kSavedPagesFile (just in case format ever needs to be
// modified).
// Version "1" supports only absolute paths
// Version "2" supports both absolute and relative paths
// Version "3" introduces columns separated by kSavedPageColumnSeparator.
//             Now "filepath<kSavedPageColumnSeparator>page title" is stored in
//             the line.
const char* kSavedPagesFileVersion = "3";

// The column separator in the savedpage metadata file, ASCII code 31 "unit
// separator" (hex 1F)
const char* kSavedPageColumnSeparator = "\x1F";

const std::string kEncodedWordUTF8QPrefix = "=?utf-8?Q?";
const std::string kEncodedWordUTF8QSuffix = "?=";

bool StringIsEncodedWordUTF8Q(const std::string& str) {
  return base::StartsWith(str, kEncodedWordUTF8QPrefix,
                          base::CompareCase::INSENSITIVE_ASCII) &&
         base::EndsWith(str, kEncodedWordUTF8QSuffix,
                        base::CompareCase::INSENSITIVE_ASCII);
}

std::string StringEncodedWordUTF8QDecode(const std::string& str) {
  std::string ret;
  std::string::size_type i = 0;
  bool between_prefix_and_suffix = false;

  while (i < str.length()) {
    if (!between_prefix_and_suffix &&
        base::StartsWith(&str[i],
                         kEncodedWordUTF8QPrefix,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      i += kEncodedWordUTF8QPrefix.length();
      between_prefix_and_suffix = true;

    } else if (between_prefix_and_suffix &&
               base::StartsWith(&str[i],
                                kEncodedWordUTF8QSuffix,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      i += kEncodedWordUTF8QSuffix.length();

      std::string::size_type space_end = i;
      while (IsAsciiWhitespace(str[space_end]))
        space_end++;
      if (base::StartsWith(&str[space_end],
                           kEncodedWordUTF8QPrefix,
                           base::CompareCase::INSENSITIVE_ASCII) == 0)
        i = space_end;  // Ignore linear-white-space (even if it includes CRLF)
                        // if between encoded words.
      between_prefix_and_suffix = false;

    } else if (between_prefix_and_suffix && str[i] == '=' &&
               IsHexDigit(str[i + 1]) && IsHexDigit(str[i + 2])) {
      ret += HexDigitToInt(str[i + 1]) * 16 + HexDigitToInt(str[i + 2]);
      i += 3;
    } else if (between_prefix_and_suffix && str[i] == '_') {
      ret += ' ';
      i++;
    } else if (!between_prefix_and_suffix && str[i] == '\r' &&
               str[i + 1] == '\n') {
      while (IsAsciiWhitespace(str[i]))
        i++;
    } else {
      ret += str[i++];
    }
  }

  return ret;
}

class MIMEKeyValue {
 public:
  MIMEKeyValue() {}
  explicit MIMEKeyValue(const std::string& string) : string_(string) {}

  bool KeyIs(const std::string& key) {
    return base::StartsWith(string_, key + ":",
                            base::CompareCase::INSENSITIVE_ASCII);
  }

  std::string GetValue() const {
    std::string::size_type pos = string_.find(':');
    if (pos == std::string::npos)
      return "";

    std::string value;
    TrimWhitespaceASCII(string_.substr(pos + 1), base::TRIM_ALL, &value);
    if (StringIsEncodedWordUTF8Q(value))
      value = StringEncodedWordUTF8QDecode(value);

    return value;
  }

  const std::string& string() { return string_; }

 private:
  std::string string_;  // Complete key/value string with trailing \r\n.
};

class MIMEHeaderReader {
 public:
  explicit MIMEHeaderReader(std::istream& stream) : stream_(stream) {}

  bool Read(MIMEKeyValue* dest) {
    std::string string;

    while (true) {
      std::string line;
      if (!std::getline(stream_, line))
        break;
      string.append(line + "\n");
      if (stream_.peek() != ' ' && stream_.peek() != '\t')
        break;
    }

    if (string.empty())
      return false;  // End of header area and start of body area.

    *dest = MIMEKeyValue(string);
    return true;
  }

 private:
  std::istream& stream_;
};

class PListReader {
 public:
  explicit PListReader(std::istream& stream) : stream_(stream) {}

  bool Open(const base::string16& key) {
    if (offsets_.empty() && !ReadRoot())
      return false;
    return Seek(key) && ReadDictionary(&dict_);
  }

  bool GetString(const base::string16& key, base::string16* value) {
    return Seek(key) && ReadString(value);
  }

 private:
  static const uint8_t kBinaryPlistMarkerInt = 0x10;
  static const uint8_t kBinaryPlistMarkerASCIIString = 0x50;
  static const uint8_t kBinaryPlistMarkerUnicode16String = 0x60;
  static const uint8_t kBinaryPlistMarkerDict = 0xD0;

  bool Seek(const base::string16& key) {
    std::map<base::string16, std::streampos>::const_iterator i(dict_.find(key));
    if (i == dict_.end())
      return false;
    stream_.seekg(i->second, stream_.beg);
    return stream_.good();
  }

  bool ReadRoot() {
    char header[8];
    stream_.seekg(0, stream_.beg);
    stream_.read(header, sizeof(header));

    stream_.seekg(-26, stream_.end);
    const uint8_t offset_entry_size = ReadU8();
    object_ref_size_ = ReadU8();
    uint64_t objects = ReadU64();
    uint64_t topObject = ReadU64();
    uint64_t offset = ReadU64();

    if (stream_.bad() || memcmp(header, "bplist0", 7) != 0 ||
        !ValidX(offset_entry_size) || !ValidX(object_ref_size_) ||
        objects < 1 || topObject >= objects || offset < 9)
      return false;

    stream_.seekg(offset);
    offsets_.clear();
    while (objects-- > 0) {
      offsets_.push_back(ReadUX(offset_entry_size));
    }

    stream_.seekg(offsets_[topObject]);
    return stream_.good() && ReadDictionary(&dict_);
  }

  bool ReadDictionary(std::map<base::string16, std::streampos>* dict) {
    uint8_t marker = ReadU8();
    if ((marker & 0xf0) != kBinaryPlistMarkerDict)
      return false;
    uint64_t count = marker & 0x0f;
    if (count == 0x0f) {
      if (!ReadInt(&count))
        return false;
    }

    if (stream_.bad())
      return false;

    dict->clear();

    std::vector<std::streampos> pos_;
    pos_.resize(count * 2);
    // First keys and then values
    for (uint8_t offset = 0; offset < 2; offset++) {
      for (uint64_t i = 0; i < count; i++) {
        uint64_t index = ReadUX(object_ref_size_);
        if (index >= offsets_.size())
          return false;
        pos_[i * 2 + offset] = offsets_[index];
      }
    }

    std::vector<std::streampos>::iterator i = pos_.begin();
    while (i != pos_.end()) {
      base::string16 key;
      stream_.seekg(*i++);
      if (!ReadString(&key)) {
        return false;
      }
      dict->insert(std::make_pair(key, *i++));
    }

    return stream_.good();
  }

  bool ReadInt(uint64_t* value) {
    uint8_t marker = ReadU8();
    if ((marker & 0xf0) != kBinaryPlistMarkerInt)
      return false;
    *value = ReadUX(1 << (marker & 0x0f));
    return stream_.good();
  }

  bool ReadString(base::string16* value) {
    uint8_t marker = ReadU8();
    bool ascii;
    switch (marker & 0xf0) {
      case kBinaryPlistMarkerASCIIString:
        ascii = true;
        break;
      case kBinaryPlistMarkerUnicode16String:
        ascii = false;
        break;
      default:
        return false;
    }
    uint64_t len = marker & 0x0f;
    if (len == 0x0f) {
      if (!ReadInt(&len))
        return false;
    }

    if (stream_.bad())
      return false;

    if (ascii) {
      char* tmp = new char[len + 1];
      stream_.read(tmp, len);
      tmp[len] = '\0';
      if (!base::IsStringASCII(tmp)) {
        delete[] tmp;
        return false;
      }
      *value = base::ASCIIToUTF16(tmp);
      delete[] tmp;
    } else {
      uint16_t* tmp = new uint16_t[len];
      stream_.read(reinterpret_cast<char*>(tmp), len * 2);
#ifdef ARCH_CPU_LITTLE_ENDIAN
      for (size_t i = 0; i < len; i++)
        tmp[i] = base::ByteSwap(tmp[i]);
#endif
      *value = base::string16(tmp, len);
      delete[] tmp;
      if (!ValidUTF16(*value))
        return false;
    }

    return stream_.good();
  }

  bool ValidUTF16(const base::string16& str) {
    const base::string16::value_type* src = str.data();
    const base::string16::value_type* const end = src + str.length();

    while (src < end) {
      uint32_t c = *src++;
      if (CBU16_IS_LEAD(c)) {
        if (src == end || !CBU16_IS_TRAIL(*src))
          return false;
        c = CBU16_GET_SUPPLEMENTARY(c, *src++);
      }
      if (!base::IsValidCharacter(c))
        return false;
    }
    return true;
  }

  uint8_t ReadU8() { return stream_.get(); }

  uint16_t ReadU16() {
    uint16_t ret;
    stream_.read(reinterpret_cast<char*>(&ret), 2);
#ifdef ARCH_CPU_LITTLE_ENDIAN
    ret = base::ByteSwap(ret);
#endif
    return ret;
  }

  uint32_t ReadU32() {
    uint32_t ret;
    stream_.read(reinterpret_cast<char*>(&ret), 4);
#ifdef ARCH_CPU_LITTLE_ENDIAN
    ret = base::ByteSwap(ret);
#endif
    return ret;
  }

  uint64_t ReadU64() {
    uint64_t ret;
    stream_.read(reinterpret_cast<char*>(&ret), 8);
#ifdef ARCH_CPU_LITTLE_ENDIAN
    ret = base::ByteSwap(ret);
#endif
    return ret;
  }

  uint64_t ReadUX(uint8_t size) {
    switch (size) {
      case 1:
        return ReadU8();
      case 2:
        return ReadU16();
      case 4:
        return ReadU32();
      case 8:
        return ReadU64();
    }
    NOTREACHED();
    return 0;
  }

  static bool ValidX(uint8_t size) {
    switch (size) {
      case 1:
      case 2:
      case 4:
      case 8:
        return true;
      default:
        return false;
    }
  }

  std::istream& stream_;
  uint8_t object_ref_size_;
  std::vector<std::streampos> offsets_;
  std::map<base::string16, std::streampos> dict_;
};

class OBMLReader {
 public:
  explicit OBMLReader(std::istream& stream) : stream_(stream) {}

  bool At(uint8_t byte) { return stream_.peek() == byte; }

  bool Skip(unsigned int bytes) {
    stream_.seekg(bytes, stream_.cur);
    return stream_.good();
  }

  bool ReadUnsignedInt(unsigned int num_bytes, uint32_t* result) {
    DCHECK(num_bytes >= 1 && num_bytes <= 4);
    *result = 0;

    while (num_bytes-- > 0) {
      int c = stream_.get();
      if (!stream_.good())
        return false;

      *result = (*result << 8) | static_cast<unsigned int>(c);
    }

    return true;
  }

  bool ReadString(std::string* dest) {
    uint32_t len;

    if (!ReadUnsignedInt(kStringLengthSize, &len))
      return false;

    dest->resize(len);
    stream_.read(&(*dest)[0], len);

    return stream_.good();
  }

  bool SkipString() {
    uint32_t len;

    if (!ReadUnsignedInt(kStringLengthSize, &len))
      return false;

    return Skip(len);
  }

 private:
  static const unsigned int kStringLengthSize = 2;

  std::istream& stream_;
};

bool ParseLine(const std::string& line,
               base::FilePath* path,
               std::string* title) {
  std::vector<std::string> columns =
      base::SplitString(line, kSavedPageColumnSeparator, base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_ALL);

  DCHECK(columns.size() >= 1);
  *path = base::FilePath(columns[0]);

  if (columns.size() >= 2) {
    *title = columns[1];
    return true;
  }

  *title = "";
  return false;
}

}  // namespace

namespace mobile {

SavedPagesReader::SavedPagesReader(
    Delegate& delegate,
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner,
    const base::FilePath& base_path,
    const base::FilePath& saved_pages_path)
    : delegate_(delegate),
      ui_task_runner_(ui_task_runner),
      file_task_runner_(file_task_runner),
      base_path_(base_path),
      saved_pages_path_(saved_pages_path),
      saved_pages_folder_(NULL) {}

SavedPagesReader::~SavedPagesReader() {}

void SavedPagesReader::ReadSavedPages() {
  if (!file_task_runner_->BelongsToCurrentThread()) {
    file_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SavedPagesReader::ReadSavedPages, base::Unretained(this)));
    return;
  }

  // to avoid problems with canonical vs non-canonical paths this set should contain only file names
  // (which should be unique as they all stored in one folder)
  std::set<base::FilePath> read_files;
  std::string saved_pages_file_content;

  if (base::ReadFileToString(base_path_.Append(kSavedPagesFile),
                             &saved_pages_file_content)) {
    std::vector<std::string> lines =
      base::SplitString(saved_pages_file_content, "\n", base::TRIM_WHITESPACE,
          base::SPLIT_WANT_ALL);

    if (lines.size() > 1 && lines[0] == kSavedPagesFileVersion) {
      for (size_t i = 1; i < lines.size(); i++) {
        if (lines[i].empty())
          continue;
        base::FilePath path;
        std::string title;

        bool had_title = ParseLine(lines[i], &path, &title);

        if (!path.IsAbsolute()) {
          path = saved_pages_path_.Append(path);
        }
        ParseSavedPage(path, had_title ? &title : NULL);
        read_files.insert(path.BaseName());
      }
    } else if (lines.size() > 1 && lines[0] == "2") {
      for (size_t i = 1; i < lines.size(); i++) {
        if (lines[i].empty())
          continue;
        base::FilePath path(lines[i]);
        if (!path.IsAbsolute()) {
          path = saved_pages_path_.Append(path);
        }
        ParseSavedPage(path);
        read_files.insert(path.BaseName());
      }
    } else if (lines.size() > 1 && lines[0] == "1") {
#if !defined(OS_ANDROID)
      // iOS changes saved_pages_path_ when upgrading
      size_t prefix = 0;
      std::vector<base::FilePath::StringType> components;
      saved_pages_path_.GetComponents(&components);
      base::FilePath prefix_path(components[prefix++]);
      for (; prefix < components.size(); prefix++) {
        prefix_path = prefix_path.Append(components[prefix]);
        if (components[prefix] == "Applications") {
          prefix_path = prefix_path.Append(components[++prefix]);
          break;
        }
      }
#endif
      for (size_t i = 1; i < lines.size(); i++) {
        if (lines[i].empty())
          continue;
        base::FilePath path = base::FilePath(lines[i]);
#if !defined(OS_ANDROID)
        path.GetComponents(&components);
        if (prefix > 0 && prefix + 1 < components.size()) {
          path = prefix_path;
          for (size_t j = prefix + 1; j < components.size(); j++) {
            path = path.Append(components[j]);
          }
        }
#endif
        ParseSavedPage(path);
        read_files.insert(path.BaseName());
      }
    }
  }

  base::FileEnumerator pages_enum(saved_pages_path_, false, base::FileEnumerator::FILES);
  for (base::FilePath name = pages_enum.Next(); !name.empty(); name = pages_enum.Next()) {
    if (read_files.find(name.BaseName()) == read_files.end()) {
      ParseSavedPage(name);
    }
  }

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SavedPagesReader::DoneReading, base::Unretained(this)));
}

void SavedPagesReader::ParseSavedPage(const base::FilePath& filename) {
  ParseSavedPage(filename, NULL);
}

void SavedPagesReader::ParseSavedPage(const base::FilePath& filename,
                                      const std::string* title) {
  if (filename.MatchesExtension(".mhtml"))
    ParseSavedPageMHTML(filename, title);
  else if (filename.MatchesExtension(".webarchive"))
    ParseSavedPageWebArchive(filename, title);
  else if (filename.MatchesExtension(".webarchivexml") ||
           filename.MatchesExtension(".mht"))
    ParseSavedPageWebView(filename, title);
  else if (filename.Extension() == "" || filename.MatchesExtension(".obml16"))
    ParseSavedPageOBML(filename, title);
  else if (filename.Extension() != ".metadata")
    ParseSavedPageFile(filename, title);
}

void SavedPagesReader::SavedPageRead(const base::string16& title,
                                     const GURL& url,
                                     const std::string& guid,
                                     const std::string& file) {
  if (!saved_pages_folder_) {
    saved_pages_folder_ = delegate_.CreateSavedPagesFolder();
    saved_pages_folder_->SetReading(true);
  }

  SavedPage* saved_page = delegate_.CreateSavedPage(title, url, guid, file);
  saved_pages_folder_->SavedPageRead(saved_page);
}

void SavedPagesReader::DoneReading() {
  if (saved_pages_folder_)
    saved_pages_folder_->SetReading(false);

  delegate_.DoneReadingSavedPages();
}

void SavedPagesReader::ParseSavedPageFile(const base::FilePath& filename) {
  ParseSavedPageFile(filename, NULL);
}

void SavedPagesReader::ParseSavedPageFile(const base::FilePath& filename,
                                          const std::string* title) {
  if (!base::PathExists(filename))
    return;
  base::string16 file_title = base::UTF8ToUTF16(filename.BaseName().value());
  GURL url = GURL(filename.value());

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SavedPagesReader::SavedPageRead,
                 base::Unretained(this),
                 !title ? file_title : base::UTF8ToUTF16(*title),
                 url,
                 filename.BaseName().RemoveExtension().value(),
                 filename.value()));
}

void SavedPagesReader::ParseSavedPageMHTML(const base::FilePath& filename) {
  ParseSavedPageMHTML(filename, NULL);
}

void SavedPagesReader::ParseSavedPageMHTML(const base::FilePath& filename,
                                           const std::string* title) {
  std::ifstream stream(filename.value().c_str());
  if (!stream.is_open())
    return;

  MIMEHeaderReader reader(stream);
  base::string16 file_title;
  GURL url;

  while (file_title.empty() || url.is_empty()) {
    MIMEKeyValue key_value;
    if (!reader.Read(&key_value))
      break;

    if (file_title.empty() && key_value.KeyIs("Subject"))
      file_title = base::UTF8ToUTF16(key_value.GetValue());
    else if (url.is_empty() && key_value.KeyIs("Content-Location"))
      url = GURL(key_value.GetValue());
  }

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SavedPagesReader::SavedPageRead,
                 base::Unretained(this),
                 !title ? file_title : base::UTF8ToUTF16(*title),
                 url,
                 filename.BaseName().RemoveExtension().value(),
                 filename.value()));
}

void SavedPagesReader::ParseSavedPageWebArchive(
    const base::FilePath& filename) {
  ParseSavedPageWebArchive(filename, NULL);
}

void SavedPagesReader::ParseSavedPageWebArchive(const base::FilePath& filename,
                                                const std::string* title) {
  std::ifstream stream(filename.value().c_str());
  if (!stream.is_open())
    return;

  PListReader reader(stream);

  base::string16 file_title;
  GURL url;

  if (!reader.Open(base::ASCIIToUTF16("WebMainResource")))
    return;

  base::string16 tmp;
  if (!reader.GetString(base::ASCIIToUTF16("WebResourceURL"), &tmp))
    return;
  url = GURL(tmp);

  if (!reader.GetString(base::ASCIIToUTF16("OperaWebResourceTitle"),
                        &file_title))
    file_title = base::EmptyString16();

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SavedPagesReader::SavedPageRead,
                 base::Unretained(this),
                 !title ? file_title : base::UTF8ToUTF16(*title),
                 url,
                 filename.BaseName().RemoveExtension().value(),
                 filename.value()));
}

void SavedPagesReader::ParseSavedPageWebView(const base::FilePath& filename) {
  ParseSavedPageWebView(filename, NULL);
}

void SavedPagesReader::ParseSavedPageWebView(const base::FilePath& filename,
                                             const std::string* title) {
  base::string16 metadata_filename =
      base::ASCIIToUTF16(filename.value()) + base::ASCIIToUTF16(".metadata");
  std::ifstream stream(UTF16ToASCII(metadata_filename).c_str());
  if (!stream.is_open())
    return;

  base::string16 file_title;
  GURL url;

  std::string tmp;
  std::getline(stream, tmp);
  url = GURL(base::UTF8ToUTF16(tmp));
  std::getline(stream, tmp);
  file_title = base::UTF8ToUTF16(tmp);

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SavedPagesReader::SavedPageRead,
                 base::Unretained(this),
                 !title ? file_title : base::UTF8ToUTF16(*title),
                 url,
                 filename.BaseName().RemoveExtension().value(),
                 filename.value()));
}

void SavedPagesReader::ParseSavedPageOBML(const base::FilePath& filename) {
  ParseSavedPageOBML(filename, NULL);
}

void SavedPagesReader::ParseSavedPageOBML(const base::FilePath& filename,
                                          const std::string* title) {
  static const uint8_t kProgressNodeMark = 0xff;
  static const unsigned int kProgressNodeSize = 2;
  static const unsigned int kEstimatedCompressedLengthSize = 3;
  static const unsigned int kCommandStreamNumBytesSize = 3;
  static const unsigned int kContentStreamNumBytesSize = 3;
  static const unsigned int kVersionSize = 1;  // Note: Only v16 is relevant.
  static const unsigned int kWSize = 2;
  static const unsigned int kHSize = 3;
  static const unsigned int kMaxAgeSize = 2;
  static const unsigned int kIconNumBytesSize = 2;

  std::ifstream stream(filename.value().c_str(), std::ifstream::binary);
  if (!stream.is_open())
    return;

  OBMLReader reader(stream);

  while (reader.At(kProgressNodeMark))
    if (!reader.Skip(kProgressNodeSize))
      return;

  uint32_t command_stream_num_bytes;
  if (!reader.Skip(kEstimatedCompressedLengthSize) ||
      !reader.ReadUnsignedInt(kCommandStreamNumBytesSize,
                              &command_stream_num_bytes) ||
      !reader.Skip(command_stream_num_bytes + kContentStreamNumBytesSize +
                   kVersionSize + kWSize + kHSize + kMaxAgeSize))
    return;

  std::string file_title;
  if (!reader.ReadString(&file_title))
    return;

  uint32_t icon_num_bytes;
  if (!reader.ReadUnsignedInt(kIconNumBytesSize, &icon_num_bytes) ||
      !reader.Skip(icon_num_bytes))
    return;

  std::string base_url, final_url;
  if (!reader.ReadString(&base_url) || !reader.ReadString(&final_url))
    return;

  if (final_url.length() == 0)
    return;
  GURL url(final_url[0] != 0 ? final_url : base_url + final_url.substr(1));

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SavedPagesReader::SavedPageRead,
                 base::Unretained(this),
                 !title ? base::UTF8ToUTF16(file_title)
                               : base::UTF8ToUTF16(*title),
                 url,
                 filename.BaseName().RemoveExtension().value(),
                 filename.value()));
}

SavedPagesWriter::SavedPagesWriter(
    const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner,
    const base::FilePath& base_path,
    const base::FilePath& saved_pages_path)
    : saved_pages_path_(saved_pages_path),
      saved_pages_folder_(NULL),
      file_writer_(base_path.Append(kSavedPagesFile),
                   file_task_runner,
                   base::TimeDelta::FromSeconds(kWriterCommitIntervalSeconds)) {
}

SavedPagesWriter::~SavedPagesWriter() { DoScheduledWrite(); }

bool SavedPagesWriter::SerializeData(std::string* data) {
  DCHECK(saved_pages_folder_);

  data->append(kSavedPagesFileVersion);
  data->append("\n");

  for (int i = 0; i < saved_pages_folder_->Size(); i++) {
    SavedPage* s = static_cast<SavedPage*>(saved_pages_folder_->Child(i));
    base::FilePath relative;
    if (saved_pages_path_.AppendRelativePath(base::FilePath(s->file()),
                                             &relative))
      data->append(relative.value());
    else
      data->append(s->file());

    *data += kSavedPageColumnSeparator;
    data->append(base::UTF16ToUTF8(s->title()));

    data->append("\n");
  }

  return true;
}

void SavedPagesWriter::DoScheduledWrite() {
  if (file_writer_.HasPendingWrite())
    file_writer_.DoScheduledWrite();
}

void SavedPagesWriter::ScheduleWrite() { file_writer_.ScheduleWrite(this); }

SavedPagesFolder::SavedPagesFolder(FavoritesImpl* favorites,
                                   int64_t parent,
                                   int64_t id,
                                   int32_t special_id,
                                   const base::string16& title,
                                   const std::string& guid,
                                   std::unique_ptr<SavedPagesWriter> writer,
                                   bool can_be_empty)
    : SpecialFolderImpl(favorites, parent, id, special_id, title, guid),
      writer_(std::move(writer)),
      reading_(false),
      can_be_empty_(can_be_empty),
      write_scheduled_while_reading_(false) {
  writer_->set_saved_pages_folder(this);
}

SavedPagesFolder::~SavedPagesFolder() {
  DCHECK(!favorites_->saved_pages_ || favorites_->saved_pages_ == this);
  favorites_->saved_pages_ = NULL;
}

bool SavedPagesFolder::CanChangeParent() const { return false; }

bool SavedPagesFolder::CanChangeTitle() const { return false; }

bool SavedPagesFolder::CanTakeMoreChildren() const { return false; }

void SavedPagesFolder::Move(int old_position, int new_position) {
  FolderImpl::Move(old_position, new_position);
  ScheduleWrite();
}

void SavedPagesFolder::Add(int pos, Favorite* favorite) {
  // TODO(marejde): Handle when a new saved page is added while still reading
  //                better. Not sure what the best approach is though...
  FolderImpl::Add(pos, favorite);
  ScheduleWrite();
}

void SavedPagesFolder::ForgetAbout(Favorite* favorite) {
  base::WeakPtr<SavedPagesFolder> ptr(AsWeakPtr());
  FolderImpl::ForgetAbout(favorite);
  if (ptr) {
    ptr->ScheduleWrite();
  }
}

void SavedPagesFolder::Remove() { RemoveChildren(); }

bool SavedPagesFolder::Check() {
  if (!can_be_empty_ && check_ == 0 && children_.empty()) {
    SpecialFolderImpl::Remove();
    return true;
  }
  return false;
}

void SavedPagesFolder::SetReading(bool reading) {
  reading_ = reading;

  if (!reading_ && write_scheduled_while_reading_) {
    write_scheduled_while_reading_ = false;
    ScheduleWrite();
  }
}

void SavedPagesFolder::SavedPageRead(SavedPage* saved_page) {
  FolderImpl::Add(Size(), saved_page);
  // Do not schedule write when reading old pages at startup.
}

void SavedPagesFolder::DoScheduledWrite() {
  if (!reading_) {
    writer_->DoScheduledWrite();
  }
}

void SavedPagesFolder::ScheduleWrite() {
  if (!reading_)
    writer_->ScheduleWrite();
  else
    write_scheduled_while_reading_ = true;
}

void SavedPagesFolder::RemoveChildren() {
  DisableCheck();
  while (Favorite* favorite = Child(0)) {
    favorite->Remove();
  }
  RestoreCheck();
}

}  // namespace mobile
