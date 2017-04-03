// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_H_
#define CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_H_

#include <string>
#include <unordered_map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/renderer/render_frame_observer.h"

#include "chill/renderer/yandex_promotion/yandex_promotion_bindings_delegate.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace opera {

class YandexPromotionBindings;

// Injects a 'private' Yandex-only JavaScript API into upon script context
// creation for documents matching a predetermined set of hosts.
class YandexPromotion : public content::RenderFrameObserver,
                        public YandexPromotionBindingsDelegate {
 public:
  explicit YandexPromotion(content::RenderFrame* render_frame);
  ~YandexPromotion() override;

 private:
  // RenderFrameObserver implementation
  bool OnMessageReceived(const IPC::Message& message) override;

  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int extension_group,
                              int world_id) override;
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int world_id) override;

  void OnDestruct() override;

  // YandexPromotionBindingsDelegate implementation
  void OnCanAskToSetAsDefault(
      std::string search_engine,
      const YandexPromotionBindingsDelegate::CanAskCallback& callback) override;
  void OnAskToSetAsDefault(std::string search_engine,
                           const base::Closure& callback) override;

  // IPC message handlers
  void OnCanAskToSetAsDefaultReply(uint32_t request_id, bool response);
  void OnAskToSetAsDefaultReply(uint32_t request_id);

  uint32_t GetNextRequestId();

  scoped_refptr<YandexPromotionBindings> bindings_;

  std::unordered_map<uint32_t, base::Callback<void(bool)>> can_ask_requests_;
  std::unordered_map<uint32_t, base::Closure> ask_requests_;

  DISALLOW_COPY_AND_ASSIGN(YandexPromotion);
};

}  // namespace opera

#endif  // CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_H_
