// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite_collection_impl.h"

#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#include "common/favorites/favorite.h"
#include "common/favorites/test_favorite_collection.h"
#include "common/favorites/favorite_collection_observer.h"
#include "common/favorites/favorite_storage.h"
#include "common/partner_content/partner_content_service.h"
#include "common/paths/opera_paths.h"
#include "common/test/base/testing_profile.h"

using base::ASCIIToUTF16;

namespace opera {
namespace {

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::SaveArg;

const FavoriteCollection::ChangeReason kReason =
    FavoriteCollection::kChangeReasonOther;
const char kTestGuid1[] = "00001111-0000-1111-0000-111100001111";
const char kTestGuid2[] = "00001111-0000-2222-0000-111100001111";
const char kTestGuid3[] = "00001111-0000-3333-0000-111100001111";
const char kFolderGuid1[] = "10001111-0000-1111-0000-111100001111";
const char kFolderGuid2[] = "20001111-0000-1111-0000-111100001111";
const char kPartnerContentGuid1[] = "00002222-0000-1111-0000-111100001111";
const char kPartnerContentGuid2[] = "00002222-0000-2222-0000-111100001111";
const char kPartnerContentGuid3[] = "00002222-0000-3333-0000-111100001111";


class MockFavoriteCollectionObserver : public FavoriteCollectionObserver {
 public:
  void FavoriteCollectionLoaded() override {}
  void FavoriteCollectionDeleted() override {}
  MOCK_METHOD3(FavoriteAdded, void(const Favorite* favorite,
                                   int new_index,
                                   FavoriteCollection::ChangeReason reason));
  MOCK_METHOD4(BeforeFavoriteRemove, void(
      const Favorite* favorite,
      const FavoriteFolder* parent,
      int index,
      FavoriteCollection::ChangeReason reason));
  MOCK_METHOD3(FavoriteChanged, void(const Favorite* favorite,
                                     const FavoriteData& old_data,
                                     FavoriteCollection::ChangeReason reason));

  MOCK_METHOD6(FavoriteMoved, void(const Favorite* favorite,
                                   const FavoriteFolder* old_parent,
                                   int old_index,
                                   const FavoriteFolder* new_parent,
                                   int new_index,
                                   FavoriteCollection::ChangeReason reason));
};

class FavoriteCollectionTest : public testing::Test {
 public:
  void SetUp() override {
    base::FilePath test_data_dir_;
    PathService::Get(base::DIR_EXE, &test_data_dir_);
    test_data_dir_ = test_data_dir_.AppendASCII("test_data_desktop");
    test_data_dir_ = test_data_dir_.AppendASCII("test_data");
    test_data_dir_ = test_data_dir_.AppendASCII("common");
    test_data_dir_ = test_data_dir_.AppendASCII("favorites");
    profile_.reset(new TestingProfile(test_data_dir_));

    collection_.reset(new TestFavoriteCollection(profile_.get()));
  }

 protected:
  FavoriteCollectionImpl& collection() const { return *collection_; }

  FavoriteFolder* AddFolder() {
    return collection().AddFavoriteFolder(
        base::string16(),
        FavoriteCollection::kIndexPositionLast,
        "",
        kReason);
  }

  FavoriteSite* AddSiteTo(FavoriteFolder* parent) {
    return collection().AddFavoriteSite(
        GURL(),
        base::string16(),
        parent,
        FavoriteCollection::kIndexPositionLast,
        "",
        kReason);
  }

  FavoriteSite* AddPartnerContentTo(FavoriteFolder* parent) {
    return collection().AddFavoritePartnerContent(
        GURL(),
        base::string16(),
        parent,
        "",
        "",
        false,
        FavoriteKeywords(),
        FavoriteCollection::kIndexPositionLast,
        kReason);
  }

  void MoveSiteTo(FavoriteSite* site, FavoriteFolder* parent) {
    collection().MoveFavorite(
        site, parent, FavoriteCollection::kIndexPositionLast, kReason);
  }

  void TestAdditionsWithExistingGuid(
      MockFavoriteCollectionObserver& observer,
      FavoriteFolder* parent) {
    EXPECT_CALL(observer, FavoriteAdded(_, _, _)).Times(0);
    Favorite* favorite = collection().AddFavoriteSite(
        GURL::EmptyGURL(),
        base::string16(),
        parent,
        FavoriteCollection::kIndexPositionLast,
        kTestGuid1,
        kReason);
    EXPECT_TRUE(!favorite);
    EXPECT_EQ(parent ? 2 : 1, collection().GetCount());
    if (parent) {
      EXPECT_EQ(0, parent->GetChildCount());
    }
    Mock::VerifyAndClearExpectations(&observer);

    if (!parent) {
      EXPECT_CALL(observer, FavoriteAdded(_, _, _)).Times(0);
      favorite = collection().AddFavoriteFolder(
          base::string16(),
          FavoriteCollection::kIndexPositionLast,
          std::string(kTestGuid1),
          kReason);

      EXPECT_TRUE(!favorite);
      EXPECT_EQ(1, collection().GetCount());
      Mock::VerifyAndClearExpectations(&observer);
    }

    EXPECT_CALL(observer, FavoriteAdded(_, _, _)).Times(0);
    favorite = collection().AddFavoritePartnerContent(
        GURL::EmptyGURL(),
        base::string16(),
        parent,
        std::string(kTestGuid1),
        std::string(),
        false,
        FavoriteKeywords(),
        FavoriteCollection::kIndexPositionLast,
        kReason);
    EXPECT_TRUE(!favorite);
    EXPECT_EQ(parent ? 2 : 1, collection().GetCount());
    if (parent) {
      EXPECT_EQ(0, parent->GetChildCount());
    }

    Mock::VerifyAndClearExpectations(&observer);
  }

 private:
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<FavoriteCollectionImpl> collection_;
};

TEST_F(FavoriteCollectionTest, AddRemoveFavorite) {
  MockFavoriteCollectionObserver observer;

  collection().AddObserver(&observer);
  InSequence sequencer;
  EXPECT_CALL(observer, FavoriteAdded(_, 0, kReason));
  EXPECT_CALL(observer, FavoriteAdded(_, 1, kReason));
  EXPECT_CALL(observer, FavoriteAdded(_, 1, kReason));
  EXPECT_CALL(observer, BeforeFavoriteRemove(_, NULL, 1, kReason));

  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               NULL,
                               0,
                               std::string(),
                               kReason);
  EXPECT_EQ(1, collection().GetCount());

  collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::string16(),
      NULL,
      FavoriteCollection::kIndexPositionLast,
      std::string(),
      kReason);
  FavoriteSite* entry = collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::string16(),
      NULL,
      1,
      std::string(),
      kReason);
  EXPECT_EQ(3, collection().GetCount());

  collection().RemoveFavorite(entry, kReason);
  EXPECT_EQ(2, collection().GetCount());
}

TEST_F(FavoriteCollectionTest, Rename) {
  FavoriteSite* entry = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                     base::string16(),
                                                     NULL,
                                                     0,
                                                     std::string(),
                                                     kReason);

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);
  EXPECT_CALL(observer, FavoriteChanged(entry, _, kReason));

  FavoriteData new_data;
  new_data.title = ASCIIToUTF16("damer");
  collection().ChangeFavorite(entry, new_data, kReason);

  EXPECT_EQ(new_data.title, entry->title());
}

TEST_F(FavoriteCollectionTest, AddRemoveFolder) {
  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);
  InSequence sequencer;
  EXPECT_CALL(observer, FavoriteAdded(_, 0, kReason));
  EXPECT_CALL(observer, FavoriteAdded(_, 1, kReason));
  EXPECT_CALL(observer, BeforeFavoriteRemove(_, NULL, 0, kReason));

  collection().AddFavoriteFolder(base::string16(),
                                 FavoriteCollection::kIndexPositionLast,
                                 std::string(),
                                 kReason);
  EXPECT_EQ(1, collection().GetCount());
  collection().AddFavoriteFolder(base::string16(),
                                 1,
                                 std::string(),
                                 kReason);
  EXPECT_EQ(2, collection().GetCount());
  collection().RemoveFavorite(collection().GetAt(0), kReason);
  EXPECT_EQ(1, collection().GetCount());
}

TEST_F(FavoriteCollectionTest, RemoveFolderWithChildren) {
  MockFavoriteCollectionObserver observer;
  FavoriteFolder* folder = collection().AddFavoriteFolder(
      base::string16(),
      0,
      std::string(),
      kReason);

  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               folder,
                               0,
                               std::string(),
                               kReason);
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               folder,
                               1,
                               std::string(),
                               kReason);

  collection().AddObserver(&observer);
  InSequence sequencer;
  EXPECT_CALL(observer, BeforeFavoriteRemove(_, folder, _, kReason));
  EXPECT_CALL(observer, BeforeFavoriteRemove(_, folder, _, kReason));
  EXPECT_CALL(observer, BeforeFavoriteRemove(folder, NULL, 0, kReason));

  collection().RemoveFavorite(collection().GetAt(0), kReason);
  EXPECT_EQ(0, collection().GetCount());
}

TEST_F(FavoriteCollectionTest, AddToFolder) {
  FavoriteFolder* folder = collection().AddFavoriteFolder(base::string16(),
                                                          0,
                                                          std::string(),
                                                          kReason);
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               NULL,
                               1,
                               std::string(),
                               kReason);

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);
  InSequence sequencer;
  EXPECT_CALL(observer, FavoriteAdded(_, 0, kReason));
  EXPECT_CALL(observer, FavoriteAdded(_, 1, kReason));
  EXPECT_CALL(observer, BeforeFavoriteRemove(_, folder, 1, kReason));

  collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::string16(),
      static_cast<FavoriteFolder*>(collection().GetAt(0)),
      0,
      std::string(),
      kReason);
  Favorite* favorite = collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::string16(),
      static_cast<FavoriteFolder*>(collection().GetAt(0)),
      FavoriteCollection::kIndexPositionLast,
      std::string(),
      kReason);
  EXPECT_EQ(2, collection().GetCount());
  EXPECT_EQ(2, folder->GetChildCount());

  collection().RemoveFavorite(favorite, kReason);
  EXPECT_EQ(2, collection().GetCount());
  EXPECT_EQ(1, folder->GetChildCount());
}

TEST_F(FavoriteCollectionTest, Move1) {
  FavoriteSite* item =
    collection().AddFavoriteSite(GURL::EmptyGURL(),
                                 base::string16(),
                                 NULL,
                                 0,
                                 std::string(),
                                 kReason);
  FavoriteFolder* folder = collection().AddFavoriteFolder(
      base::string16(),
      1,
      std::string(),
      kReason);
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               NULL,
                               2,
                               std::string(),
                               kReason);
  collection().MoveFavorite(item,
                            folder,
                            0,
                            kReason);
  EXPECT_EQ(2, collection().GetCount());
}

TEST_F(FavoriteCollectionTest, Move2) {
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               NULL,
                               0,
                               std::string(),
                               kReason);
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               NULL,
                               1,
                               std::string(),
                               kReason);
  FavoriteSite* item = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                    base::string16(),
                                                    NULL,
                                                    2,
                                                    std::string(),
                                                    kReason);
  collection().MoveFavorite(item,
                            NULL,
                            1,
                            kReason);
  EXPECT_EQ(3, collection().GetCount());
}

TEST_F(FavoriteCollectionTest, MovingUsingPrevGuidInRoot) {
  FavoriteSite* f1 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  NULL,
                                                  0,
                                                  std::string(),
                                                  kReason);
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               NULL,
                               1,
                               std::string(),
                               kReason);
  FavoriteSite* f3 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  NULL,
                                                  2,
                                                  std::string(),
                                                  kReason);

  EXPECT_TRUE(collection().MoveFavorite(f1, NULL, f3->guid(), kReason));
  EXPECT_EQ(3, collection().GetCount());
  Favorite* fav = collection().GetAt(2);
  EXPECT_EQ(f1, fav);

  EXPECT_TRUE(collection().MoveFavorite(f3, NULL, "", kReason));
  EXPECT_EQ(3, collection().GetCount());
  fav = collection().GetAt(0);
  EXPECT_EQ(f3, fav);
}

TEST_F(FavoriteCollectionTest, MovingUsingPrevGuidInFolder) {
  FavoriteFolder* folder = collection().AddFavoriteFolder(
      base::string16(),
      0,
      std::string(),
      kReason);

  FavoriteSite* f1 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  folder,
                                                  0,
                                                  std::string(),
                                                  kReason);
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               folder,
                               1,
                               std::string(),
                               kReason);
  FavoriteSite* f3 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  folder,
                                                  2,
                                                  std::string(),
                                                  kReason);

  EXPECT_TRUE(collection().MoveFavorite(f1, folder, f3->guid(), kReason));
  EXPECT_EQ(1, collection().GetCount());
  EXPECT_EQ(3, folder->GetChildCount());
  Favorite* fav = folder->GetChildByIndex(2);
  EXPECT_EQ(f1, fav);

  EXPECT_TRUE(collection().MoveFavorite(f3, folder, "", kReason));
  EXPECT_EQ(1, collection().GetCount());
  EXPECT_EQ(3, folder->GetChildCount());
  fav = folder->GetChildByIndex(0);
  EXPECT_EQ(f3, fav);
}

TEST_F(FavoriteCollectionTest,
       MovingUsingPrevGuidFromFolderToRootAndToFolderAgain) {
  FavoriteFolder* folder = collection().AddFavoriteFolder(base::string16(),
                                                          0,
                                                          std::string(),
                                                          kReason);

  FavoriteSite* f1 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  folder,
                                                  0,
                                                  std::string(),
                                                  kReason);
  FavoriteSite* f2 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  folder,
                                                  1,
                                                  std::string(),
                                                  kReason);
  FavoriteSite* f3 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  folder,
                                                  2,
                                                  std::string(),
                                                  kReason);

  EXPECT_TRUE(collection().MoveFavorite(f1, NULL, "", kReason));
  EXPECT_EQ(2, collection().GetCount());
  EXPECT_EQ(2, folder->GetChildCount());
  Favorite* fav = collection().GetAt(0);
  EXPECT_EQ(f1, fav);

  EXPECT_TRUE(collection().MoveFavorite(f3, NULL, f1->guid(), kReason));
  EXPECT_EQ(3, collection().GetCount());
  EXPECT_EQ(1, folder->GetChildCount());
  fav = collection().GetAt(1);
  EXPECT_EQ(f3, fav);

  EXPECT_TRUE(collection().MoveFavorite(f1, folder, f2->guid(), kReason));
  EXPECT_EQ(2, collection().GetCount());
  EXPECT_EQ(2, folder->GetChildCount());
  fav = folder->GetChildByIndex(1);
  EXPECT_EQ(f1, fav);
}

TEST_F(FavoriteCollectionTest, MovingFailsOnInvalidPrevGuid) {
  FavoriteSite* f1 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  NULL,
                                                  0,
                                                  std::string(),
                                                  kReason);
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               NULL,
                               0,
                               std::string(),
                               kReason);

  EXPECT_FALSE(collection().MoveFavorite(f1, NULL, "abracadabra", kReason));
  EXPECT_EQ(2, collection().GetCount());
}

TEST_F(FavoriteCollectionTest, MovingFailsOnPrevGuidBeingTheGuidOfMoved) {
  FavoriteSite* f1 = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                  base::string16(),
                                                  NULL,
                                                  0,
                                                  std::string(),
                                                  kReason);
  collection().AddFavoriteSite(GURL::EmptyGURL(),
                               base::string16(),
                               NULL,
                               0,
                               std::string(),
                               kReason);

  EXPECT_FALSE(collection().MoveFavorite(f1, NULL, f1->guid(), kReason));
  EXPECT_EQ(2, collection().GetCount());
}

TEST_F(FavoriteCollectionTest, Iterate) {
  for (int i = 0; i < 10; i++)
    collection().AddFavoriteSite(GURL::EmptyGURL(),
                                 base::string16(),
                                 NULL,
                                 0,
                                 std::string(),
                                 kReason);
  int count = collection().GetCount();
  EXPECT_EQ(count, 10);
  Favorite* favorite = NULL;
  for (int i = 0; i < 1; i++) {
    favorite = collection().GetAt(i);
    EXPECT_TRUE(favorite != NULL);
  }
}

TEST_F(FavoriteCollectionTest, GetAt) {
  FavoriteSite* f0 = collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::string16(),
      NULL,
      0,
      std::string(),
      kReason);
  FavoriteSite* f1 = collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::string16(),
      NULL,
      1,
      std::string(),
      kReason);
  FavoriteSite* f2 = collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::string16(),
      NULL,
      2,
      std::string(),
      kReason);
  DCHECK_EQ(f0, collection().GetAt(0));
  DCHECK_EQ(f1, collection().GetAt(1));
  DCHECK_EQ(f2, collection().GetAt(2));
}

TEST_F(FavoriteCollectionTest, LiveTitleAndURLChangeWhenTitleAndURLChange) {
  FavoriteExtension* favorite = collection().AddFavoriteExtension(
      "", FavoriteExtensionState(), NULL, 0, kReason);

  FavoriteData new_data(favorite->data());
  new_data.title = ASCIIToUTF16("new title");
  new_data.url = GURL("http://new.url.com");
  collection().ChangeFavorite(favorite, new_data, kReason);

  EXPECT_EQ(new_data.title, favorite->live_title());
  EXPECT_EQ(new_data.url, favorite->live_url());
}

TEST_F(FavoriteCollectionTest, OwnedByUser_Root) {
  FavoriteFolder* folder = collection().AddFavoriteFolder(
      base::string16(), 0, "", kReason);
  EXPECT_TRUE(folder->data_owned_by_user());

  FavoriteSite* fpc = collection().AddFavoritePartnerContent(
      GURL(), base::string16(), NULL, "", "", true, FavoriteKeywords(), 0, kReason);
  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_FALSE(fpc->data_owned_by_user());

  FavoriteSite* fpc2 = collection().AddFavoritePartnerContent(
      GURL(), base::string16(), NULL, "", "", true, FavoriteKeywords(), 0, kReason);
  EXPECT_FALSE(fpc->data_owned_by_user());

  FavoriteSite* fs = collection().AddFavoriteSite(
      GURL(), base::string16(), NULL, 0, "", kReason);
  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(fs->data_owned_by_user());
  EXPECT_FALSE(fpc->data_owned_by_user());
  EXPECT_FALSE(fpc2->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, RegularSiteAddedToEmptyFolder) {
  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);
  EXPECT_CALL(observer, FavoriteAdded(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(observer, FavoriteChanged(_, _, _)).Times(0);

  FavoriteFolder* folder = AddFolder();
  FavoriteSite* site = AddSiteTo(folder);
  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(site->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, PartnerSiteAddedToEmptyFolder) {
  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  InSequence sequencer;

  EXPECT_CALL(observer, FavoriteAdded(_, _, _));

  FavoriteFolder* folder = AddFolder();
  ASSERT_TRUE(folder->data_owned_by_user());

  EXPECT_CALL(observer, FavoriteChanged(folder, _, _));
  EXPECT_CALL(observer, FavoriteAdded(_, _, _));

  FavoriteSite* partner = AddPartnerContentTo(folder);

  EXPECT_FALSE(folder->data_owned_by_user());
  EXPECT_FALSE(partner->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, RegularSiteMovedToEmptyFolder) {
  FavoriteSite* site = AddSiteTo(NULL);
  FavoriteFolder* folder = AddFolder();

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);
  EXPECT_CALL(observer, FavoriteMoved(_, _, _, _, _, _));
  EXPECT_CALL(observer, FavoriteChanged(_, _, _)).Times(0);

  MoveSiteTo(site, folder);

  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(site->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, PartnerSiteMovedToEmptyFolder) {
  FavoriteSite* partner = AddPartnerContentTo(NULL);
  FavoriteFolder* folder = AddFolder();
  ASSERT_TRUE(folder->data_owned_by_user());

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  InSequence sequencer;
  EXPECT_CALL(observer, FavoriteChanged(folder, _, _));
  EXPECT_CALL(observer, FavoriteMoved(partner, NULL, _, folder, _, _));

  MoveSiteTo(partner, folder);

  EXPECT_FALSE(folder->data_owned_by_user());
  EXPECT_FALSE(partner->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, RegularSiteAddedToFolderWithPartnerSites) {
  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  InSequence sequencer;

  EXPECT_CALL(observer, FavoriteAdded(_, _, _));
  FavoriteFolder* folder = AddFolder();

  EXPECT_CALL(observer, FavoriteChanged(folder, _, _));
  EXPECT_CALL(observer, FavoriteAdded(_, _, _));
  EXPECT_CALL(observer, FavoriteAdded(_, _, _));
  FavoriteSite* partner0 = AddPartnerContentTo(folder);
  FavoriteSite* partner1 = AddPartnerContentTo(folder);
  ASSERT_FALSE(folder->data_owned_by_user());

  EXPECT_CALL(observer, FavoriteChanged(folder, _, _));
  EXPECT_CALL(observer, FavoriteChanged(partner0, _, _));
  EXPECT_CALL(observer, FavoriteChanged(partner1, _, _));
  EXPECT_CALL(observer, FavoriteAdded(_, _, _));

  FavoriteSite* site = AddSiteTo(folder);

  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(partner0->data_owned_by_user());
  EXPECT_TRUE(partner1->data_owned_by_user());
  EXPECT_TRUE(site->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, PartnerSiteAddedToFolderWithRegularSites) {
  FavoriteFolder* folder = AddFolder();
  FavoriteSite* site0 = AddSiteTo(folder);
  FavoriteSite* site1 = AddSiteTo(folder);
  ASSERT_TRUE(folder->data_owned_by_user());

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  InSequence sequencer;
  EXPECT_CALL(observer, FavoriteAdded(_, _, _));
  EXPECT_CALL(observer, FavoriteChanged(_, _, _));

  FavoriteSite* partner = AddPartnerContentTo(folder);

  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(site0->data_owned_by_user());
  EXPECT_TRUE(site1->data_owned_by_user());
  EXPECT_TRUE(partner->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, RegularSiteMovedToFolderWithPartnerSites) {
  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  InSequence sequencer;

  EXPECT_CALL(observer, FavoriteAdded(_, _, _));
  FavoriteFolder* folder = AddFolder();
  EXPECT_CALL(observer, FavoriteChanged(folder, _, _));
  EXPECT_CALL(observer, FavoriteAdded(_, _, _));
  EXPECT_CALL(observer, FavoriteAdded(_, _, _));
  FavoriteSite* partner0 = AddPartnerContentTo(folder);
  FavoriteSite* partner1 = AddPartnerContentTo(folder);

  EXPECT_CALL(observer, FavoriteAdded(_, _, _));
  FavoriteSite* site = AddSiteTo(NULL);

  ASSERT_FALSE(folder->data_owned_by_user());

  EXPECT_CALL(observer, FavoriteChanged(folder, _, _));
  EXPECT_CALL(observer, FavoriteChanged(partner0, _, _));
  EXPECT_CALL(observer, FavoriteChanged(partner1, _, _));
  EXPECT_CALL(observer, FavoriteMoved(site, NULL, _, folder, _, _));

  MoveSiteTo(site, folder);

  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(partner0->data_owned_by_user());
  EXPECT_TRUE(partner1->data_owned_by_user());
  EXPECT_TRUE(site->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, PartnerSiteMovedToFolderWithRegularSites) {
  FavoriteFolder* folder = AddFolder();
  FavoriteSite* site0 = AddSiteTo(folder);
  FavoriteSite* site1 = AddSiteTo(folder);
  FavoriteSite* partner = AddPartnerContentTo(NULL);
  ASSERT_TRUE(folder->data_owned_by_user());

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  InSequence sequencer;
  EXPECT_CALL(observer, FavoriteMoved(partner, NULL, _, folder, _, _));
  EXPECT_CALL(observer, FavoriteChanged(partner, _, _));

  MoveSiteTo(partner, folder);

  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(site0->data_owned_by_user());
  EXPECT_TRUE(site1->data_owned_by_user());
  EXPECT_TRUE(partner->data_owned_by_user());
}

TEST_F(FavoriteCollectionTest, PartnerSiteInFolderAndURLChanged) {
  FavoriteFolder* folder = AddFolder();
  FavoriteSite* partner0 = AddPartnerContentTo(folder);
  FavoriteSite* changed_partner = AddPartnerContentTo(folder);
  FavoriteSite* partner1 = AddPartnerContentTo(folder);
  ASSERT_FALSE(folder->data_owned_by_user());

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  InSequence sequencer;
  EXPECT_CALL(observer, FavoriteChanged(folder, _, _));
  EXPECT_CALL(observer, FavoriteChanged(partner0, _, _));
  EXPECT_CALL(observer, FavoriteChanged(partner1, _, _));
  EXPECT_CALL(observer, FavoriteChanged(changed_partner, _, _));

  FavoriteData new_data(changed_partner->data());
  new_data.url = GURL("http://something.else");
  collection().ChangeFavorite(
      changed_partner, new_data, FavoriteCollection::kChangeReasonUser);

  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(partner0->data_owned_by_user());
  EXPECT_TRUE(partner1->data_owned_by_user());
  EXPECT_TRUE(changed_partner->data_owned_by_user());
  EXPECT_FALSE(changed_partner->data().redirect);
}

TEST_F(FavoriteCollectionTest, PartnerContentSerialization) {
  const std::string pid = "partnerid";

  FavoriteKeywords keywords;
  keywords.push_back(ASCIIToUTF16("ala"));
  keywords.push_back(ASCIIToUTF16("ma"));
  keywords.push_back(ASCIIToUTF16("kota"));

  FavoriteSite* fpc = collection().AddFavoritePartnerContent(
      GURL(), base::string16(), NULL, std::string(), pid, true, keywords, 0, kReason);

  base::Pickle pickle;
  fpc->data().Serialize(&pickle);

  FavoriteData deserialized_data;
  deserialized_data.Deserialize(pickle);
  FavoriteSite fpc2("", deserialized_data);

  EXPECT_FALSE(fpc2.data_owned_by_user());

  const FavoriteKeywords& keywords2 = fpc2.GetKeywords();
  EXPECT_EQ(keywords, keywords2);

  EXPECT_EQ(pid, fpc2.partner_id());
  EXPECT_EQ(PartnerContentService::GetPartnerRedirectUrl(pid),
            fpc2.navigate_url());
}

// Changing the title only must not make it "owned by user".
TEST_F(FavoriteCollectionTest, ChangePartnerContentTitle) {
  FavoriteFolder* folder = collection().AddFavoriteFolder(
      base::string16(), 0, std::string(), kReason);
  EXPECT_TRUE(folder->data_owned_by_user());

  FavoriteSite* fpc = collection().AddFavoritePartnerContent(
      GURL(),
      UTF8ToUTF16("first title"),
      folder,
      std::string(),
      "partner id 1",
      true,
      FavoriteKeywords(),
      0,
      kReason);
  EXPECT_FALSE(folder->data_owned_by_user());

  FavoriteSite* fpc2 = collection().AddFavoritePartnerContent(
      GURL(),
      UTF8ToUTF16("second title"),
      folder,
      std::string(),
      "partner id 2",
      true,
      FavoriteKeywords(),
      0,
      kReason);
  EXPECT_FALSE(folder->data_owned_by_user());

  FavoriteData new_data(fpc2->data());
  new_data.title = ASCIIToUTF16("autoupdate changed title");
  collection().ChangeFavorite(fpc2,
                              new_data,
                              FavoriteCollection::kChangeReasonAutoupdate);

  EXPECT_FALSE(folder->data_owned_by_user());
  EXPECT_FALSE(fpc->data_owned_by_user());
  EXPECT_FALSE(fpc2->data_owned_by_user());

  new_data.title = ASCIIToUTF16("user changed title");
  collection().ChangeFavorite(fpc2,
                              new_data,
                              FavoriteCollection::kChangeReasonUser);

  EXPECT_FALSE(folder->data_owned_by_user());
  EXPECT_FALSE(fpc->data_owned_by_user());
  EXPECT_FALSE(fpc2->data_owned_by_user());
}

// Changing the URL must make it "owned by user" if and only if changed by the
// user.
TEST_F(FavoriteCollectionTest, ChangePartnerContentUrl) {
  // This should set redirect to false
  FavoriteFolder* folder = collection().AddFavoriteFolder(
      base::string16(), 0, std::string(), kReason);
  EXPECT_TRUE(folder->data_owned_by_user());

  FavoriteSite* fpc = collection().AddFavoritePartnerContent(
      GURL(),
      UTF8ToUTF16("first title"),
      folder,
      std::string(),
      "test_partner",
      true,
      FavoriteKeywords(),
      0,
      kReason);
  EXPECT_FALSE(folder->data_owned_by_user());

  // Autoupdate changes visible URL
  FavoriteData new_data(fpc->data());
  new_data.url = GURL("http://updated.url");
  collection().ChangeFavorite(fpc,
                              new_data,
                              FavoriteCollection::kChangeReasonAutoupdate);
  EXPECT_FALSE(folder->data_owned_by_user());
  EXPECT_FALSE(fpc->data_owned_by_user());
  EXPECT_EQ(PartnerContentService::GetPartnerRedirectUrl("test_partner"),
            fpc->navigate_url());

  // User changes url
  new_data.url = GURL("http://foo.br");
  collection().ChangeFavorite(fpc,
                              new_data,
                              FavoriteCollection::kChangeReasonUser);
  EXPECT_TRUE(folder->data_owned_by_user());
  EXPECT_TRUE(fpc->data_owned_by_user());
  EXPECT_EQ(new_data.url, fpc->navigate_url());
}

TEST_F(FavoriteCollectionTest, NotAllowingSameGuid1) {
  // The initial favorite with a conflicting guid is in root level.

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  EXPECT_CALL(observer, FavoriteAdded(_, 0, kReason));
  Favorite* favorite = collection().AddFavoriteSite(GURL::EmptyGURL(),
                                                    base::string16(),
                                                    NULL,
                                                    0,
                                                    kTestGuid1,
                                                    kReason);
  ASSERT_TRUE(!!favorite);
  ASSERT_EQ(1, collection().GetCount());
  Mock::VerifyAndClearExpectations(&observer);

  TestAdditionsWithExistingGuid(observer, NULL);

  EXPECT_CALL(observer, FavoriteAdded(_, 1, kReason));
  favorite = collection().AddFavoriteFolder(
      base::string16(),
      FavoriteCollection::kIndexPositionLast,
      std::string(kFolderGuid1),
      kReason);
  ASSERT_TRUE(!!favorite);
  ASSERT_EQ(Favorite::kFavoriteFolder, favorite->GetType());
  ASSERT_EQ(2, collection().GetCount());
  Mock::VerifyAndClearExpectations(&observer);

  TestAdditionsWithExistingGuid(
      observer, static_cast<FavoriteFolder*>(favorite));
}

TEST_F(FavoriteCollectionTest, NotAllowingSameGuid2) {
  // The initial favorite with a conflicting guid is in a folder.

  MockFavoriteCollectionObserver observer;
  collection().AddObserver(&observer);

  EXPECT_CALL(observer, FavoriteAdded(_, 0, kReason)).Times(2);
  FavoriteFolder* folder = collection().AddFavoriteFolder(
      base::string16(),
      FavoriteCollection::kIndexPositionLast,
      std::string(kFolderGuid1),
      kReason);
  ASSERT_TRUE(!!folder);
  ASSERT_EQ(1, collection().GetCount());
  FavoriteSite* favorite = collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::string16(),
      folder,
      FavoriteCollection::kIndexPositionLast,
      std::string(kTestGuid1),
      kReason);
  ASSERT_TRUE(!favorite);
  ASSERT_EQ(1, folder->GetChildCount());
  Mock::VerifyAndClearExpectations(&observer);

  TestAdditionsWithExistingGuid(observer, NULL);

  EXPECT_CALL(observer, FavoriteAdded(_, 1, kReason));
  folder = collection().AddFavoriteFolder(
      base::string16(),
      FavoriteCollection::kIndexPositionLast,
      std::string(kFolderGuid2),
      kReason);
  ASSERT_TRUE(!!folder);
  ASSERT_EQ(2, collection().GetCount());
  Mock::VerifyAndClearExpectations(&observer);

  TestAdditionsWithExistingGuid(observer, folder);
}

TEST_F(FavoriteCollectionTest, FilterFavoritePartnerContent) {
  FavoriteFolder* folder_should_stay = collection().AddFavoriteFolder(
      base::ASCIIToUTF16("Normal folder"),
      FavoriteCollection::kIndexPositionLast,
      std::string(kFolderGuid1),
      kReason);

  FavoriteSite* site_should_stay = collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::ASCIIToUTF16("Site in normal folder"),
      folder_should_stay,
      FavoriteCollection::kIndexPositionLast,
      std::string(kTestGuid1),
      kReason);

  FavoriteSite* site_should_stay2 = collection().AddFavoriteSite(
      GURL::EmptyGURL(),
      base::ASCIIToUTF16("Site in root folder"),
      NULL,
      FavoriteCollection::kIndexPositionLast,
      std::string(kTestGuid2),
      kReason);

  FavoriteFolder* partner_folder = collection().AddFavoriteFolder(
      base::ASCIIToUTF16("Partner folder"),
      FavoriteCollection::kIndexPositionLast,
      std::string(kPartnerContentGuid1),
      kReason);

  collection().AddFavoritePartnerContent(
      GURL::EmptyGURL(),
      base::ASCIIToUTF16("Partner site in partner folder"),
      partner_folder,
      std::string(kPartnerContentGuid2),
      "partner_id1",
      false,
      FavoriteKeywords(),
      FavoriteCollection::kIndexPositionLast,
      kReason);

  FavoriteSite* partner_site2 = collection().AddFavoritePartnerContent(
      GURL::EmptyGURL(),
      base::ASCIIToUTF16("Partner site in root folder"),
      NULL,
      std::string(kPartnerContentGuid3),
      "partner_id2",
      false,
      FavoriteKeywords(),
      FavoriteCollection::kIndexPositionLast,
      kReason);

  base::hash_set<std::string> to_stay;
  // Partner site in root folder stays this time.
  to_stay.insert(partner_site2->guid());
  collection().FilterFavoritePartnerContent(to_stay);

  // First partner site got filtered out.
  EXPECT_EQ(NULL, collection().FindByGUID(kPartnerContentGuid2));
  // Second stays due to 'to_stay'.
  EXPECT_EQ(partner_site2, collection().FindByGUID(partner_site2->guid()));
  // Folder becomes empty after partner_site1 is removed, so it's gone too.
  EXPECT_EQ(NULL, collection().FindByGUID(kPartnerContentGuid1));
  // All non-partner stuff stays.
  EXPECT_EQ(folder_should_stay,
            collection().FindByGUID(folder_should_stay->guid()));
  EXPECT_EQ(site_should_stay,
            collection().FindByGUID(site_should_stay->guid()));
  EXPECT_EQ(site_should_stay2,
            collection().FindByGUID(site_should_stay2->guid()));
}

TEST_F(FavoriteCollectionTest,
       FilterFavoritePartnerContent_KeepNonEmptyfolders) {
  FavoriteFolder* partner_folder = collection().AddFavoriteFolder(
      base::ASCIIToUTF16("Partner folder"),
      FavoriteCollection::kIndexPositionLast,
      std::string(kPartnerContentGuid1),
      kReason);

  collection().AddFavoritePartnerContent(
      GURL::EmptyGURL(),
      base::ASCIIToUTF16("Partner site in partner folder"),
      partner_folder,
      std::string(kPartnerContentGuid2),
      "partner_id1",
      false,
      FavoriteKeywords(),
      FavoriteCollection::kIndexPositionLast,
      kReason);

  FavoriteSite* partner_site2 = collection().AddFavoritePartnerContent(
      GURL::EmptyGURL(),
      base::ASCIIToUTF16("Partner site in partner folder"),
      partner_folder,
      std::string(kPartnerContentGuid3),
      "partner_id2",
      false,
      FavoriteKeywords(),
      FavoriteCollection::kIndexPositionLast,
      kReason);

  base::hash_set<std::string> to_stay;
  to_stay.insert(partner_site2->guid());
  collection().FilterFavoritePartnerContent(to_stay);

  // First partner site got filtered out.
  EXPECT_TRUE(!collection().FindByGUID(kPartnerContentGuid2));
  // Second stays due to 'to_stay'.
  EXPECT_EQ(partner_site2,
            collection().FindByGUID(partner_site2->guid()));
  // Folder stays because it still has second partner site inside
  EXPECT_EQ(partner_folder, collection().FindByGUID(partner_folder->guid()));
}

}  // namespace
}  // namespace opera
