/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * Copyright (C) 2017 Opera Software AS.  All rights reserved.
 *
 * This file is an original work developed by Opera Software.
 */

#include "pch.h"

#include <jni.h>
#include "jni_obfuscation.h"
#include "jniutils.h"

#include "bream.h"
#include "mini_stl.h"
#include "platform.h"
#include "vminstance_ext.h"

#include "pushedcontent.h"

#include "common/favorites/favorites_delegate.h"

OP_EXTERN_C_BEGIN

#include "mini/c/shared/core/components/DynamicContent.h"
#include "mini/c/shared/core/obml/ObmlCommands/ObmlCommand.h"
#include "mini/c/shared/core/pi/DynamicContentClient.h"
#include "mini/c/shared/core/util/Vector.h"

#include "com_opera_android_PushedContentHandler.h"
#include "com_opera_android_PushedSpeedDialHandler.h"

OP_EXTERN_C_END

namespace {

using mobile::FavoritesDelegate;

#define DISALLOW_COPY_AND_ASSIGNMENT(CLSNAME) \
  CLSNAME(const CLSNAME &) = delete; \
  CLSNAME &operator=(const CLSNAME &) = delete

struct PartnerContentImage {
  PartnerContentImage(const void *image, UINT32 imageLen);
  ~PartnerContentImage();

  const int8_t *data;
  size_t length;

 private:
  DISALLOW_COPY_AND_ASSIGNMENT(PartnerContentImage);
};

const int32_t kInvalidChannel = -1;

// Information about speed dial group as it comes from the server.
struct PartnerContentGroupInfo {
  PartnerContentGroupInfo(const ObmlSpeedDialV2GroupInfo *info);

  int32_t id;
  mstd::string title;
  mstd::vector<int32_t> channels;

 private:
  DISALLOW_COPY_AND_ASSIGNMENT(PartnerContentGroupInfo);
};

// Base type for speed dial items that can be created in favorites.
struct PartnerContentItem {
  virtual ~PartnerContentItem() {}

  virtual void Create(
      FavoritesDelegate::PartnerContentInterface *favorites,
      int64_t parent, int position) = 0;

  int32_t id;
  int32_t channel;
  mstd::string title;
};

// Speed dial group after processing information from server.
// Describes hierarchy of items ready to be created.
struct PartnerContentGroup : public PartnerContentItem {
  PartnerContentGroup(const PartnerContentGroupInfo &group_info);

  void Create(
      FavoritesDelegate::PartnerContentInterface *favorites,
      int64_t parent, int position) override;

  mstd::vector<PartnerContentItem *> children;

 private:
  DISALLOW_COPY_AND_ASSIGNMENT(PartnerContentGroup);
};

// Speed dial leaf entry.
struct PartnerContentEntry : public PartnerContentItem {
  PartnerContentEntry(const ObmlSpeedDialV2EntryInfo *info);

  void Create(
      FavoritesDelegate::PartnerContentInterface *favorites,
      int64_t parent, int position) override;

  PartnerContentImage thumbnail;
  mstd::string url;
  int32_t checksum;

 private:
  DISALLOW_COPY_AND_ASSIGNMENT(PartnerContentEntry);
};

class PartnerContentHandlerImpl : public mobile::PartnerContentHandler {
 public:
  PartnerContentHandlerImpl()
      : favorites_(NULL),
        thumbnail_size_(0),
        initial_content_push_(true) {}

  void SetPartnerContentInterface(
      FavoritesDelegate::PartnerContentInterface *interface) override {
    favorites_ = interface;
  }

  void SetThumbnailSize(int thumbnail_size) override {
    MOP_ASSERT(thumbnail_size > 0);
    thumbnail_size_ = static_cast<size_t>(thumbnail_size);
  }

  void OnPartnerContentActivated() override;
  void OnPartnerContentChanged(int32_t partner_id) override;

  void ProcessContentPush(const ObmlSpeedDialPushV2Info *info);
  void ResetActivationCounts();

  void SetPushHeader();
  void SetUsageHeader(uint8_t sequence_number);
  void SetClearedHeader(const jint *deleted, jsize length);


  void set_initial_content_push(bool initial) {
    initial_content_push_ = initial;
  }

 private:
  // Represents partner content transaction information pushed
  // from the server and being processed by the client.
  struct PartnerContentPush {
    PartnerContentPush(const ObmlSpeedDialPushV2Info *info);
    ~PartnerContentPush();

    mstd::vector<int32_t> removed_channels;
    mstd::vector<PartnerContentGroupInfo *> groups;
    mstd::vector<PartnerContentEntry *> entries;

   private:
    DISALLOW_COPY_AND_ASSIGNMENT(PartnerContentPush);
  };

  void ProcessPushedRemovals(const mstd::vector<int32_t> &removedChannels);

  FavoritesDelegate::PartnerContentInterface *favorites_;
  size_t thumbnail_size_;
  uint8_t usage_sequence_number_;
  bool initial_content_push_;
};

template<typename T>
void AddUnique(mstd::vector<T> &v, const T &item) {
  // Push item to vector but only if it's not already there.
  // The expected problem size is too small to bother with anything fancy
  // here.
  for (auto i = v.begin(); i != v.end(); ++i) {
    if (*i == item) return;
  }
  v.push_back(item);
}

// Try to find and remove first occurrence. Returns true if an
// occurrence was actually found.
template<typename T>
bool FindAndRemove(mstd::vector<T> &v, const T &item) {
  for (auto i = v.begin(); i != v.end(); ++i) {
    if (*i == item) {
      v.remove(i);
      return true;
    }
  }
  return false;
}

// Pushes integer as VLQ into data. The number will be encoded
// as 1-5 bytes. MSB in each byte indicates if next one should be
// used.
void WriteIntAsVlq(mstd::vector<UINT8> &data, int32_t integer) {
  bool continuation = false;
  uint32_t value = (uint32_t)integer;
  auto position = data.end();
  do {
    UINT8 byte = static_cast<UINT8>(value & 0x7f);
    value >>= 7;
    if (continuation) {
      byte |= 0x80;
    } else {
      continuation = true;
    }
    data.insert(position, byte);
  } while (value != 0);
}

// A list of locally created pointers that should be automatically
// deleted when exiting scope. In other words, an autorelease pool.
template<typename T>
class ScopedPointerList {
 public:
  ~ScopedPointerList() {
    for (auto i = pointers_.begin(); i != pointers_.end(); ++i) {
      delete *i;
    }
  }

  T *add(T *pointer) {
    pointers_.push_back(pointer);
    return pointer;
  }
 private:
  typename mstd::vector<T *> pointers_;
};

// Wraps an array allocated on heap in order to make sure its
// memory will be released.
template<typename T>
class ScopedArray {
 public:
  ScopedArray(T *array) : array_(array) {}
  ~ScopedArray() { if (array_) delete[] array_; }

  explicit operator bool() const { return array_ != nullptr; }
  T &operator[](int index) { return array_[index]; }
 private:
  T *array_;
};

PartnerContentHandlerImpl *g_partner_content_handler =
    new PartnerContentHandlerImpl();

}  // namespace

namespace mobile {

PartnerContentHandler *partnerContentHandler() {
  return g_partner_content_handler;
}

}  // namespace mobile

jclass g_pushedcontenthandler_class;

#define USE_METHOD(NAME)                            \
  static jmethodID g_pushedcontenthandler_ ## NAME

USE_METHOD(nativeChannelUpdate);
USE_METHOD(resetClearedSpeedDials);
USE_METHOD(resetSpeedDialUsage);
USE_METHOD(removePushedSpeedDial);

#undef USE_METHOD

void initPushedContent(JNIEnv *env) {
  scoped_localref<jclass> clazz(
      env, env->FindClass(C_com_opera_android_PushedContentHandler));
  g_pushedcontenthandler_class = (jclass)env->NewGlobalRef(*clazz);

#define PREPARE_METHOD(NAME) \
  g_pushedcontenthandler_##NAME = env->GetStaticMethodID( \
      *clazz, M_com_opera_android_PushedContentHandler_##NAME##_ID)

  PREPARE_METHOD(nativeChannelUpdate);
  PREPARE_METHOD(resetClearedSpeedDials);
  PREPARE_METHOD(resetSpeedDialUsage);
  PREPARE_METHOD(removePushedSpeedDial);

#undef PREPARE_METHOD
}

#define CALL_METHOD(ENV, NAME, ...)                                 \
  (ENV)->CallStaticVoidMethod(                                      \
      g_pushedcontenthandler_class, g_pushedcontenthandler_##NAME,  \
      ##__VA_ARGS__)

void Java_com_opera_android_PushedContentHandler_setDynamicContentVersion(
    __unused JNIEnv *env, jclass, jint channelId, jint version) {
  MOpDynamicContent_setChecksum(MOpDynamicContent_get(), channelId, version);
}

void Java_com_opera_android_PushedContentHandler_setInitialContentPush(
    __unused JNIEnv *env, jclass, jboolean initial) {
  g_partner_content_handler->set_initial_content_push(initial);
}

void Java_com_opera_android_PushedSpeedDialHandler_setPushHeader(
    __unused JNIEnv *env, jclass) {
  g_partner_content_handler->SetPushHeader();
}

void Java_com_opera_android_PushedSpeedDialHandler_setUsageHeader(
    __unused JNIEnv *env, jclass, jint sequenceNumber) {
  MOP_ASSERT(sequenceNumber >= 0 && sequenceNumber < 256);
  g_partner_content_handler->SetUsageHeader(
      static_cast<uint8_t>(sequenceNumber));
}

void Java_com_opera_android_PushedSpeedDialHandler_setClearedSpeedDialsHeader(
    JNIEnv *env, jclass, jintArray deletedIds) {
  jsize length = env->GetArrayLength(deletedIds);
  jint *deleted = env->GetIntArrayElements(deletedIds, 0);
  g_partner_content_handler->SetClearedHeader(deleted, length);
}

void MOpDynamicContentClient_newChannelPayload(
    UINT8 channelId, INT32 checksum,
    const UINT8 *data, size_t dataLen) {
  JNIEnv *env = getEnv();
  jbyteArray payload = env->NewByteArray(dataLen);
  env->SetByteArrayRegion(payload, 0, dataLen, (const jbyte *)data);
  CALL_METHOD(env, nativeChannelUpdate, channelId, checksum, payload);
}

void MOpDynamicContentClient_speedDialPushV2(const ObmlSpeedDialPushV2Info *info) {
  g_partner_content_handler->ProcessContentPush(info);
}

void MOpDynamicContentClient_resetClearedSpeedDialsV2() {
  CALL_METHOD(getEnv(), resetClearedSpeedDials);
}

void MOpDynamicContentClient_resetSpeedDialV2Usage() {
  g_partner_content_handler->ResetActivationCounts();
  CALL_METHOD(getEnv(), resetSpeedDialUsage);
}

PartnerContentImage::PartnerContentImage(const void *image, UINT32 imageLen)
    : data((const int8_t *)image), length(imageLen) {
  // Currently we process everything synchronously within OBML stream
  // delivered by Reksio so there's no need for copying data around.
  // Images are potentially significant in size, after all.
}

PartnerContentImage::~PartnerContentImage() {}

PartnerContentGroupInfo::PartnerContentGroupInfo(
    const ObmlSpeedDialV2GroupInfo *info) {
  id = info->id;
  title.assign(info->title, info->titleSize);
  channels.reserve(info->channelCount);
  for (int i = 0; i < info->channelCount; ++i) {
    channels.push_back(info->channels[i]);
  }
}

PartnerContentGroup::PartnerContentGroup(
    const PartnerContentGroupInfo &group_info) {
  id = group_info.id;
  channel = kInvalidChannel;
  title = group_info.title;
}

void PartnerContentGroup::Create(
    FavoritesDelegate::PartnerContentInterface *favorites,
    int64_t, int position) {
  int64_t folder = favorites->CreateFolder(position, title.c_str(),
                                           NULL, 0, id);
  int childIndex = 0;
  for (auto i = children.begin(); i != children.end(); ++i) {
    (*i)->Create(favorites, folder, childIndex++);
  }
}

PartnerContentEntry::PartnerContentEntry(
    const ObmlSpeedDialV2EntryInfo *info)
    : thumbnail(info->image, info->imageLen),
      checksum(info->checksum) {
  id = info->id;
  channel = info->channel;
  title.assign(info->title, info->titleSize);
  url.assign(info->url, info->urlSize);
}

void PartnerContentEntry::Create(
    FavoritesDelegate::PartnerContentInterface *favorites,
    int64_t parent, int position) {
  favorites->CreatePartnerContent(
      parent, position, title.c_str(), url.c_str(),
      thumbnail.data, thumbnail.length,
      channel, id, checksum);
}

PartnerContentHandlerImpl::PartnerContentPush::PartnerContentPush(
    const ObmlSpeedDialPushV2Info *info) {
  removed_channels.reserve(info->removedCount);
  for (int i = 0; i < info->removedCount; ++i) {
    removed_channels.push_back(info->removed[i]);
  }

  if (info->groups) {
    groups.reserve(MOpVector_size(info->groups));
    for (auto i = 0; i < MOpVector_size(info->groups); ++i) {
      ObmlSpeedDialV2GroupInfo *group_info =
          reinterpret_cast<ObmlSpeedDialV2GroupInfo *>(
              MOpVector_get(info->groups, i));
      groups.push_back(new PartnerContentGroupInfo(group_info));
    }
  }

  if (info->entries) {
    entries.reserve(MOpVector_size(info->entries));
    for (auto i = 0; i < MOpVector_size(info->entries); ++i) {
      ObmlSpeedDialV2EntryInfo *entry_info =
          reinterpret_cast<ObmlSpeedDialV2EntryInfo *>(
              MOpVector_get(info->entries, i));
      entries.push_back(new PartnerContentEntry(entry_info));
    }
  }
}

PartnerContentHandlerImpl::PartnerContentPush::~PartnerContentPush() {
  for (auto i = entries.begin(); i != entries.end(); ++i) {
    delete *i;
  }

  for (auto i = groups.begin(); i != groups.end(); ++i) {
    delete *i;
  }
}

void PartnerContentHandlerImpl::ProcessPushedRemovals(
    const mstd::vector<int32_t> &removedChannels) {
  mstd::vector<int64_t> parents;
  for (auto i = removedChannels.begin(); i != removedChannels.end(); ++i) {
    int64_t entry = favorites_->FindEntryFromPushChannel(*i);
    if (entry != favorites_->GetRoot()) {
      int64_t parent = favorites_->GetParent(entry);
      favorites_->RemovePartnerContent(entry);

      // TODO: check if this is necessary.
      AddUnique(parents, parent);
    }
  }
  for (auto i = parents.begin(); i != parents.end(); ++i) {
    favorites_->UpdatePushedGroupId(*i);
  }
}

void PartnerContentHandlerImpl::ProcessContentPush(
    const ObmlSpeedDialPushV2Info *info) {
  // What follows is the actual processing of pushed partner content, also
  // known as speed dials. The code follows closely the Bream implementation
  // from FavoriteSource.bream. The specification of how speed dials should be
  // processed can be found on the wiki at:
  // https://wiki.opera.com/display/PC/Client+side+implementation+of+Speed+Dial+v2#ClientsideimplementationofSpeedDialv2-Algorithmforprocessingthepayload
  //
  // In short, we first remove entries scheduled for removal, and then go
  // through all the other entries. The ones existing on the client are
  // updated accordingly. Remaining are queued for addition.
  //
  // Next we iterate through group information. If group already exists, its
  // new entries are pulled from the queue and immediately created.
  //
  // If group does not exist, its entries are pulled from the queue and
  // grouped in a new object representing the group. This is then inserted
  // back into the queue at position of first removed entry.
  //
  // At this point all the existing items are updated, and queue comprises all
  // the items, simple entries and groups, that need to be created.

  // Parse the C structure, preparing data for passing to the favorites store.
  // This also serves as a temporary work space for working with collections
  // received from the server using more convenient APIs.
  PartnerContentPush push(info);

  ProcessPushedRemovals(push.removed_channels);

  // Handles releasing memory of any group objects created during processing.
  // Those would be added to the add queue for processing, and so need to
  // remain alive as long as the add_list.
  ScopedPointerList<PartnerContentGroup> group_release_pool;
  // Queue for items that will actually be added, simple or compound.
  mstd::vector<PartnerContentItem *> add_list;

  // Go through pushed entries.
  for (auto i = push.entries.begin(); i != push.entries.end(); ++i) {
    PartnerContentEntry *entry = *i;
    int64_t old_entry = favorites_->FindEntryFromPushChannel(entry->channel);
    if (old_entry != favorites_->GetRoot()) {
      // This entry already exists on the client, and will not be immediately
      // updated, resetting activation count if its ID changed.
      int32_t activation_count = entry->id != favorites_->GetPushId(old_entry) ?
          0 : favorites_->GetPushActivationCount(old_entry);
      favorites_->SetPushData(old_entry, entry->channel, entry->id,
                              entry->checksum, activation_count);
      favorites_->UpdatePartnerContent(
          old_entry, entry->title.c_str(), entry->url.c_str(),
          entry->thumbnail.data, entry->thumbnail.length);

      // Remove the entry from groups in the transaction, as it was already
      // processed here, so searching it in the add queue in the future would
      // always fail.
      for (auto g = push.groups.begin(); g != push.groups.end(); ++g) {
        // Channel can be only in 1 group, so bail out as soon as an
        // occurrence is found in one of them.
        if (FindAndRemove((*g)->channels, entry->channel)) {
          break;
        }
      }
    } else {
      // A new entry that has to be added, to the root, existing group, or a
      // brand new group. We don't know that yet.
      add_list.push_back(entry);
    }
  }

  // Go through pushed groups.
  for (auto g = push.groups.begin(); g != push.groups.end(); ++g) {
    PartnerContentGroupInfo *group_info = *g;
    int64_t old_group = favorites_->FindFolderFromPushId(group_info->id);
    if (old_group != favorites_->GetRoot()) {
      // Group exists on the client. Add the content of the transaction
      // group to the existing one, pulling matching entries from the add
      // queue, and removing them from the queue in the process.
      int index = favorites_->GetChildCount(old_group);
      for (auto channel = group_info->channels.begin();
           channel != group_info->channels.end();
           ++channel) {
        for (auto item = add_list.begin(); item != add_list.end(); ++item) {
          if ((*item)->channel == *channel) {
            (*item)->Create(favorites_, old_group, index++);
            add_list.remove(item);
            break;
          }
        }
      }
    } else if (group_info->channels.size() > 1) {
      // Skip groups with one item. Those are already on the add_list in form
      // of their singular entries, which is sufficient, as we do not allow
      // groups with single items.
      //
      // Otherwise, we need to create a new group and insert it in the add
      // queue at the position of its first entry, moving all of its entries
      // from the add_list into the group itself.
      PartnerContentGroup *group = group_release_pool.add(
          new PartnerContentGroup(*group_info));
      bool inserted = false;
      for (auto channel = group_info->channels.begin();
           channel != group_info->channels.end();
           ++channel) {
        // Find the item in the add queue.
        for (auto item = add_list.begin(); item != add_list.end(); ++item) {
          if ((*item)->channel == *channel) {  // Bingo.
            group->children.push_back(*item);  // Move into the group.
            if (!inserted) {
              // If this is the first item of the group in the add queue,
              // it needs to be replaced with the group itself.
              *item = group;
              inserted = true;
            } else {
              // Otherwise, just remove it from the queue, as it has been moved
              // into the group that is already queued.
              add_list.remove(item);
            }
            break;
          }
        }
      }
      MOP_ASSERT(group->children.size() == group_info->channels.size());
      MOP_ASSERT(inserted);
    }
  }

  // add_list now contains the groups and entries that we need to add,
  // in the desired order.
  int index = 0;
  if (initial_content_push_) {
    // Initially, pushed speed dials are inserted first.
    initial_content_push_ = false;
  } else {
    // Later, new entries are added at the end, after any user entries.
    index = favorites_->GetChildCount(favorites_->GetRoot());
  }
  for (auto item = add_list.begin(); item != add_list.end(); ++item) {
    (*item)->Create(favorites_, favorites_->GetRoot(), index++);
  }

  SetPushHeader();
  SetUsageHeader(usage_sequence_number_);
}

void PartnerContentHandlerImpl::ResetActivationCounts() {
  favorites_->ResetActivationCountForAllPartnerContent();
}

void PartnerContentHandlerImpl::OnPartnerContentActivated() {
  SetUsageHeader(usage_sequence_number_);
}

void PartnerContentHandlerImpl::OnPartnerContentChanged(int32_t partner_id) {
  CALL_METHOD(getEnv(), removePushedSpeedDial, partner_id);
}

// Format of headers is described in the mini server documentation. See
// doc/request_format.txt. The code below follows closely the Bream
// implementation found in PushedSpeedDialV2Manager.bream.

void PartnerContentHandlerImpl::SetPushHeader() {
  MOP_ASSERT(favorites_);
  if (!favorites_) return;  // Can't anything without access to favorites.

  size_t size = 0, count = 0;
  ScopedArray<int32_t> header_data(
      favorites_->GetHeaderDataForAllPartnerContent(&size));

  MOP_ASSERT(thumbnail_size_ > 0);
  mstd::vector<UINT8> header;
  header.reserve(5 + 4 * size);
  // Thumbnail size, 2 + 2 bytes.
  header.push_back(0xff & (thumbnail_size_ >> 8));
  header.push_back(0xff & thumbnail_size_);
  header.push_back(0xff & (thumbnail_size_ >> 8));
  header.push_back(0xff & thumbnail_size_);
  // Number of speed dials.
  MOP_ASSERT(size % 3 == 0);
  count = size / 3;
  MOP_ASSERT(count < 256);
  header.push_back(static_cast<UINT8>(0xff & count));
  for (size_t i = 0, e = 0; e < count; ++e) {
    // 1 byte of channel.
    header.push_back(static_cast<UINT8>(header_data[i++]));
    // 1-5 bytes of speed dial ID.
    WriteIntAsVlq(header, header_data[i++]);
    // 2 bytes of speed dial ID.
    uint32_t checksum = static_cast<uint32_t>(header_data[i++]);
    header.push_back(static_cast<UINT8>((0x0000ff00 & checksum) >> 8));
    header.push_back(static_cast<UINT8>(0x000000ff & checksum));
  }
  MOpDynamicContent_setSpeedDialPushV2Header(
      MOpDynamicContent_get(), header.data(), header.size());
}

void PartnerContentHandlerImpl::SetUsageHeader(uint8_t sequence_number) {
  MOP_ASSERT(favorites_);
  if (!favorites_) return;  // Can't anything without access to favorites.

  size_t size = 0, count = 0;
  ScopedArray<int32_t> usage_data(
      favorites_->GetUsageHeaderDataForAllPartnerContent(&size));

  count = size / 2;
  MOP_ASSERT(size % 2 == 0);
  MOP_ASSERT(count <= 255);
  mstd::vector<UINT8> header;

  if (count > 0) {
    header.reserve(3 + count * 5);
    // Reserved byte, currently 0.
    header.push_back(0);
    // Sequence number.
    header.push_back(sequence_number);
    // Number of data units.
    header.push_back(static_cast<UINT8>(0xff & count));
    // For each data unit, 4-byte ID and 1-byte activation count
    for (size_t i = 0; i < size;) {
      uint32_t id = static_cast<uint32_t>(usage_data[i++]);
      header.push_back(static_cast<UINT8>((0xff000000 & id) >> 24));
      header.push_back(static_cast<UINT8>((0x00ff0000 & id) >> 16));
      header.push_back(static_cast<UINT8>((0x0000ff00 & id) >> 8));
      header.push_back(static_cast<UINT8>(0x000000ff & id));

      uint32_t activation_count = static_cast<uint32_t>(usage_data[i++]);
      header.push_back(static_cast<UINT8>(0xff & activation_count));
    }
  }
  usage_sequence_number_ = sequence_number;
  MOpDynamicContent_setSpeedDialV2UsageHeader(
      MOpDynamicContent_get(), header.data(), header.size());
}

void PartnerContentHandlerImpl::SetClearedHeader(const jint *deleted,
                                                 jsize length) {
  if (length < 1) {
    MOpDynamicContent_setClearedSpeedDialsV2(MOpDynamicContent_get(), NULL, 0);
    return;
  }

  MOP_ASSERT(favorites_);
  if (!favorites_) return;  // Can't anything without access to favorites.
  MOP_ASSERT(length < 256);
  mstd::vector<UINT8> header;
  // The header has 2 parts with identical format: deleted items, followed
  // by converted items. We no longer support the latter, so their count is
  // always equal to 0.
  // The format is: 1-byte length, 4-byte id for each item.
  header.reserve(2 + length * 4);

  // Deleted items.
  header.push_back(static_cast<UINT8>(0xff & static_cast<size_t>(length)));
  for (auto i = 0; i < length; ++i) {
    uint32_t id = static_cast<uint32_t>(deleted[i]);
    header.push_back(static_cast<UINT8>((0xff000000 & id) >> 24));
    header.push_back(static_cast<UINT8>((0x00ff0000 & id) >> 16));
    header.push_back(static_cast<UINT8>((0x0000ff00 & id) >> 8));
    header.push_back(static_cast<UINT8>(0x000000ff & id));
  }
  // No converted items.
  header.push_back(0);

  MOpDynamicContent_setClearedSpeedDialsV2(
      MOpDynamicContent_get(), header.data(), header.size());
}
