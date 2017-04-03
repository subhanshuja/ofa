// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <algorithm>
#include <functional>
#include <list>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"

#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_item.h"
#include "common/suggestion/suggestion_manager.h"
#include "common/suggestion/suggestion_provider.h"
#include "common/suggestion/suggestion_tokens.h"

using namespace opera;
using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Matcher;
using ::testing::Not;
using ::testing::PrintToString;

namespace opera {
bool operator==(const SuggestionItem& item1, const SuggestionItem& item2) {
  return item1.relevance == item2.relevance &&
      item1.title == item2.title &&
      item1.url == item2.url &&
      item1.type == item2.type;
}

void PrintTo(const SuggestionItem& item, std::ostream* ostream) {
  *ostream << ::std::endl << "SuggestionItem {" << std::endl <<
      "\tint relevance=" << item.relevance << std::endl <<
      "\tstd::string title=" << item.title << std::endl <<
      "\tstd::string url=" << item.url << std::endl <<
      "\tstd::string type=" << item.type << std::endl <<
      "}";
}
}  // namespace opera

MATCHER_P(SizeIs, value, "") {
  *result_listener << "actual size is " << arg.size();
  return arg.size() == value;
}

class MockProvider : public SuggestionProvider {
 public:
  virtual ~MockProvider() {}
  MOCK_METHOD4(Query,
               void(const SuggestionTokens&,
                    bool,
                    const std::string&,
                    const SuggestionCallback&));
  MOCK_METHOD0(Cancel, void());
};

class CallbackInterface {
 public:
  virtual void Callback(const std::vector<SuggestionItem>& suggestions) = 0;
};

class MockCallback : CallbackInterface {
 public:
  MOCK_METHOD1(Callback, void(const std::vector<SuggestionItem>&));
};

class SuggestionManagerTest : public ::testing::Test {
 public:
  SuggestionManagerTest() {
    manager_ = new SuggestionManager(message_loop_.task_runner());
  }

  ~SuggestionManagerTest() {
    message_loop_.RunUntilIdle();
    delete manager_;
    manager_ = NULL;
  }

  void Query(const SuggestionTokens& query) {
    manager_->Query(query, false, base::Bind(&MockCallback::Callback,
                                             base::Unretained(&callback_)));
  }

 protected:
  void AddProvider(std::unique_ptr<SuggestionProvider> provider) {
    manager_->AddProvider(provider.Pass(), "TEST");
  }

  MockCallback& callback() {
    return callback_;
  }

  scoped_refptr<base::SingleThreadTaskRunner> task_runner() {
    return message_loop_.task_runner();
  }

 private:
  SuggestionManager* manager_;
  MockCallback callback_;
  base::MessageLoop message_loop_;
};

TEST_F(SuggestionManagerTest, QueryProvider) {
  std::unique_ptr<MockProvider> provider(new MockProvider);

  EXPECT_CALL(*provider.get(), Query(SuggestionTokens("query"), _, _, _));
  EXPECT_CALL(*provider.get(), Cancel());

  AddProvider(provider.Pass());

  Query(SuggestionTokens("query"));
}

TEST_F(SuggestionManagerTest, MultiQueryProvider) {
  std::unique_ptr<MockProvider> provider(new MockProvider);

  EXPECT_CALL(*provider.get(),
              Query(SuggestionTokens("query1"), _, _, _));
  EXPECT_CALL(*provider.get(),
              Query(SuggestionTokens("query2"), _, _, _));
  EXPECT_CALL(*provider.get(), Cancel()).Times(2);

  AddProvider(provider.Pass());

  Query(SuggestionTokens("query1"));
  Query(SuggestionTokens("query2"));
}

TEST_F(SuggestionManagerTest, QueryProviders) {
  std::unique_ptr<MockProvider> provider1(new MockProvider);
  std::unique_ptr<MockProvider> provider2(new MockProvider);
  std::unique_ptr<MockProvider> provider3(new MockProvider);

  EXPECT_CALL(*provider1.get(), Query(SuggestionTokens("query"), _, _, _));
  EXPECT_CALL(*provider2.get(), Query(SuggestionTokens("query"), _, _, _));
  EXPECT_CALL(*provider3.get(), Query(SuggestionTokens("query"), _, _, _));

  EXPECT_CALL(*provider1.get(), Cancel());
  EXPECT_CALL(*provider2.get(), Cancel());
  EXPECT_CALL(*provider3.get(), Cancel());

  AddProvider(provider1.Pass());
  AddProvider(provider2.Pass());
  AddProvider(provider3.Pass());

  Query(SuggestionTokens("query"));
}

class DummyProvider : public SuggestionProvider {
 public:
  template<typename InputIterator>
  DummyProvider(const scoped_refptr<base::MessageLoopProxy>& loop,
                InputIterator begin, InputIterator end)
      : message_loop_proxy_(loop) {
    while (begin != end) {
      suggestions_.push_back(*begin++);
    }
  }
  virtual ~DummyProvider() {}
  virtual void Query(
      const SuggestionTokens& query,
      bool /* private_browsing */,
      const std::string&, /* type */
      const SuggestionCallback& callback) {
    message_loop_proxy_->PostTask(FROM_HERE,
                                  base::Bind(
                                      &DummyProvider::AsyncQuery,
                                      base::Unretained(this), callback));
  }

  virtual void Cancel() {
    // TODO(mage): Here we should cancel the pending query.
  }

  void AddSuggestion(const SuggestionItem& item) {
    suggestions_.push_back(item);
    std::sort(suggestions_.begin(), suggestions_.end(),
              std::not2(std::less<SuggestionItem>()));
  }

  const std::vector<SuggestionItem>& GetSuggestions() {
    return suggestions_;
  }

  void ClearSuggestions() {
    suggestions_ = std::vector<SuggestionItem>();
  }


 private:
  void AsyncQuery(const SuggestionCallback& callback) {
    callback.Run(suggestions_);
  }

  scoped_refptr<base::MessageLoopProxy> message_loop_proxy_;
  std::vector<SuggestionItem> suggestions_;
};

TEST_F(SuggestionManagerTest, QueryProviderAnswer) {
  SuggestionItem items[] = {
    SuggestionItem(0, "a", "http://www.a.com", "URL")
  };

  std::unique_ptr<DummyProvider> provider(
      new DummyProvider(message_loop(), &items[0], &items[0] + 1));

  EXPECT_CALL(callback(), Callback(AllOf(
      SizeIs(1), ElementsAre(items[0]))));

  ASSERT_EQ(1, provider.get()->GetSuggestions().size());

  AddProvider(provider.Pass());

  Query(SuggestionTokens("query"));
}

TEST_F(SuggestionManagerTest, QueryProviderAnswers) {
  SuggestionItem items[] = { SuggestionItem(3, "a", "http://www.a.com", "URL"),
                             SuggestionItem(2, "b", "http://www.b.com", "URL"),
                             SuggestionItem(1, "c", "http://www.c.com", "URL")
  };

  std::unique_ptr<DummyProvider> provider(
      new DummyProvider(message_loop(), &items[0], &items[0] + 3));

  EXPECT_CALL(callback(), Callback(
      AllOf(SizeIs(3), ElementsAreArray(items))));

  ASSERT_EQ(3, provider.get()->GetSuggestions().size());

  AddProvider(provider.Pass());

  Query(SuggestionTokens("query"));
}

TEST_F(SuggestionManagerTest, SuggestionManagerQueryMerge) {
  SuggestionItem items[] = { SuggestionItem(6, "a", "http://www.a.com", "URL"),
                             SuggestionItem(5, "b", "http://www.b.com", "URL"),
                             SuggestionItem(4, "c", "http://www.c.com", "URL"),
                             SuggestionItem(3, "d", "http://www.d.com", "URL"),
                             SuggestionItem(2, "e", "http://www.e.com", "URL"),
                             SuggestionItem(1, "f", "http://www.f.com", "URL")
  };

  std::unique_ptr<DummyProvider> provider0(
      new DummyProvider(message_loop(), &items[0], &items[0] + 3));
  std::unique_ptr<DummyProvider> provider1(
      new DummyProvider(message_loop(), &items[3], &items[3] + 3));

  EXPECT_CALL(callback(), Callback(
      ElementsAreArray(&items[0], 6)));
  EXPECT_CALL(callback(), Callback(
      ElementsAreArray(&items[0], 3)));

  ASSERT_EQ(3, provider0.get()->GetSuggestions().size());
  ASSERT_EQ(3, provider1.get()->GetSuggestions().size());

  AddProvider(provider0.Pass());
  AddProvider(provider1.Pass());

  Query(SuggestionTokens("query"));
}

TEST_F(SuggestionManagerTest, SuggestionManagerQueryMergeUnique) {
  SuggestionItem items[] = {
    SuggestionItem(6, "a", "http://www.a.com", "URL"),
    SuggestionItem(5, "b", "http://www.b.com", "URL"),
    SuggestionItem(4, "c", "http://www.c.com", "URL"),
    SuggestionItem(3, "d", "http://www.a.com", "OTHER"),
    SuggestionItem(2, "e", "http://www.b.com", "OTHER"),
    SuggestionItem(1, "f", "http://www.c.com", "OTHER")
  };

  std::unique_ptr<DummyProvider> provider0(
      new DummyProvider(message_loop(), &items[0], &items[0] + 3));
  std::unique_ptr<DummyProvider> provider1(
      new DummyProvider(message_loop(), &items[3], &items[3] + 3));

  EXPECT_CALL(callback(), Callback(
      ElementsAreArray(&items[0], 3))).Times(2);

  ASSERT_EQ(3, provider0.get()->GetSuggestions().size());
  ASSERT_EQ(3, provider1.get()->GetSuggestions().size());

  AddProvider(provider0.Pass());
  AddProvider(provider1.Pass());

  Query(SuggestionTokens("query"));
}

ACTION_P2(RunCallback, provider, smt) {
  provider->ClearSuggestions();
  ASSERT_EQ(0, provider->GetSuggestions().size());
  smt->Query(SuggestionTokens("query"));
}

TEST_F(SuggestionManagerTest, SuggestionManagerQueryMergeRemove) {
  SuggestionItem items[] = {
    SuggestionItem(6, "a", "http://www.a.com", "URL"),
    SuggestionItem(5, "b", "http://www.b.com", "URL")
  };

  std::unique_ptr<DummyProvider> provider(
      new DummyProvider(message_loop(), &items[0], &items[0] + 2));
  DummyProvider* p = provider.get(); // D:

  EXPECT_CALL(callback(), Callback(SizeIs(0)))
      .Times(1)
      .After(EXPECT_CALL(callback(), Callback(Not(SizeIs(0))))
                .Times(1)
                .WillOnce(RunCallback(p, this)));

  ASSERT_EQ(2, provider.get()->GetSuggestions().size());

  AddProvider(provider.Pass());

  Query(SuggestionTokens("query"));
}

TEST_F(SuggestionManagerTest, SuggestionTokensTrim) {
  SuggestionTokens query("  hello\n  world \xE2\x81\x9F");
  EXPECT_EQ("hello\n  world", query.phrase());
}

TEST_F(SuggestionManagerTest, SuggestionTokensTokenization) {
  SuggestionTokens query("  hello world\tthis\n\"is a\" \xE2\x81\x9F"
                         "  \xC2\xABtest ""\xC2\xBB \xE5\x83\x96\xE5\x83\x9A ");
  ASSERT_EQ(7, query.terms().size());
  std::list<std::string>::const_iterator w = query.terms().begin();
  EXPECT_EQ("hello", *w++);
  EXPECT_EQ("world", *w++);
  EXPECT_EQ("this", *w++);
  EXPECT_EQ("is a", *w++);
  EXPECT_EQ("test ", *w++);
  EXPECT_EQ("\xE5\x83\x96", *w++);  // 僖
  EXPECT_EQ("\xE5\x83\x9A", *w++);  // 僚
}

TEST_F(SuggestionManagerTest, SuggestionTokensLowerCase) {
  SuggestionTokens query("ABc dEF \"\xC3\x85\xC3\x84\xC3\x96\"");
  ASSERT_EQ(3, query.terms().size());
  std::list<std::string>::const_iterator w = query.terms().begin();
  EXPECT_EQ("abc", *w++);
  EXPECT_EQ("def", *w++);
  EXPECT_EQ("\xC3\xA5\xC3\xA4\xC3\xB6", *w++);
}

TEST_F(SuggestionManagerTest, SuggestionTokensEquality) {
  {
    SuggestionTokens query1("1");
    SuggestionTokens query2(" 1    ");
    EXPECT_EQ(query1, query2);
  }
  {
    SuggestionTokens query1;
    SuggestionTokens query2("  ");
    EXPECT_EQ(query1, query2);
  }
  {
    SuggestionTokens query1("KALLE");
    SuggestionTokens query2("kalle");
    EXPECT_NE(query1, query2);
  }
}
