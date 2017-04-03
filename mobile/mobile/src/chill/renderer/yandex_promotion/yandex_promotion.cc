// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/renderer/yandex_promotion/yandex_promotion.h"

#include <unordered_set>
#include <utility>

#include "base/strings/string_util.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

#include "chill/common/yandex_promotion_messages.h"
#include "chill/renderer/yandex_promotion/yandex_promotion_bindings.h"

namespace {

const std::unordered_set<std::string> kAllowedYandexHosts{
    "hamster.yandex.by",
    "hamster.yandex.com.tr",
    "hamster.yandex.kz",
    "hamster.yandex.ru",
    "hamster.yandex.ua",
    "opera.hamster.yandex.by",
    "opera.hamster.yandex.com.tr",
    "opera.hamster.yandex.kz",
    "opera.hamster.yandex.ru",
    "opera.hamster.yandex.ua",
    "www.yandex.by",
    "www.yandex.com.tr",
    "www.yandex.com",
    "www.yandex.kz",
    "www.yandex.net",
    "www.yandex.ru",
    "www.yandex.ua",
    "www.yandex.uz",
    "ya.ru",
    "yandex.by",
    "yandex.com.tr",
    "yandex.com",
    "yandex.kz",
    "yandex.net",
    "yandex.ru",
    "yandex.ua",
    "yandex.uz",
    "zelo.serp.yandex.by",
    "zelo.serp.yandex.com.tr",
    "zelo.serp.yandex.kz",
    "zelo.serp.yandex.ru",
    "zelo.serp.yandex.ua",
};

bool IsUrlAllowedBindings(const GURL& host_url) {
  if (!host_url.SchemeIs(url::kHttpsScheme))
    return false;

  return kAllowedYandexHosts.find(host_url.host()) != kAllowedYandexHosts.end();
}

const int kMainWorldId = 0;

}  // namespace

namespace opera {

YandexPromotion::YandexPromotion(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  DCHECK(render_frame->IsMainFrame())
      << "Private Yandex API not available in subframes";
}

YandexPromotion::~YandexPromotion() {}

void YandexPromotion::OnDestruct() {
  delete this;
}

bool YandexPromotion::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(YandexPromotion, message)
    IPC_MESSAGE_HANDLER(YandexMsg_CanAskToSetAsDefaultReply,
                        OnCanAskToSetAsDefaultReply)
    IPC_MESSAGE_HANDLER(YandexMsg_AskToSetAsDefaultReply,
                        OnAskToSetAsDefaultReply)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void YandexPromotion::DidCreateScriptContext(v8::Local<v8::Context> context,
                                             int extension_group,
                                             int world_id) {
  if (extension_group != 0 || world_id != kMainWorldId)
    return;

  blink::WebDocument document = render_frame()->GetWebFrame()->document();
  GURL document_url(document.url());
  if (!IsUrlAllowedBindings(document_url))
    return;

  VLOG(1) << "Injecting bindings for " << document_url.spec();
  bindings_ = new YandexPromotionBindings(this, context);
}

void YandexPromotion::WillReleaseScriptContext(v8::Local<v8::Context> context,
                                               int world_id) {
  if (world_id != kMainWorldId)
    return;

  can_ask_requests_.clear();
  ask_requests_.clear();

  // Reset reference count, destroying the bindings object
  bindings_ = scoped_refptr<YandexPromotionBindings>();
}

void YandexPromotion::OnCanAskToSetAsDefault(
    std::string search_engine,
    const YandexPromotionBindingsDelegate::CanAskCallback& callback) {
  auto request = std::make_pair(GetNextRequestId(), callback);
  can_ask_requests_.insert(request);
  Send(new YandexMsg_CanAskToSetAsDefault(routing_id(), request.first,
                                          search_engine));
}

void YandexPromotion::OnAskToSetAsDefault(std::string search_engine,
                                          const base::Closure& callback) {
  uint32_t request_id = GetNextRequestId();
  if (!callback.is_null()) {
    ask_requests_.insert(std::make_pair(request_id, callback));
  }

  Send(
      new YandexMsg_AskToSetAsDefault(routing_id(), request_id, search_engine));
}

void YandexPromotion::OnCanAskToSetAsDefaultReply(uint32_t request_id,
                                                  bool response) {
  auto it = can_ask_requests_.find(request_id);
  if (it == can_ask_requests_.end()) {
    // Can happen if script context is released while browser does its thing
    return;
  }

  base::Callback<void(bool)> callback = it->second;
  can_ask_requests_.erase(it);

  callback.Run(response);
}

void YandexPromotion::OnAskToSetAsDefaultReply(uint32_t request_id) {
  auto it = ask_requests_.find(request_id);
  if (it == ask_requests_.end()) {
    // Can happen if script context is released while browser does its thing, or
    // if no callback was provided.
    return;
  }

  base::Closure callback = it->second;
  ask_requests_.erase(it);

  callback.Run();
}

uint32_t YandexPromotion::GetNextRequestId() {
  static uint32_t request_id = 0;
  return ++request_id;
}

}  // namespace opera
