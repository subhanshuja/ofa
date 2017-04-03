// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_BINDINGS_H_
#define CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_BINDINGS_H_

#include "base/memory/ref_counted.h"
#include "v8/include/v8.h"

namespace opera {

class YandexPromotionBindingsDelegate;

// Yandex-specific ES bindings, adding an 'opr' property to the global (window)
// object.
class YandexPromotionBindings
    : public base::RefCounted<YandexPromotionBindings> {
 public:
  // Does not assume ownership over |delegate|
  YandexPromotionBindings(YandexPromotionBindingsDelegate* delegate,
                          v8::Local<v8::Context> v8_context);

 private:
  void Inject(v8::Isolate* isolate);

  static void canAskToSetAsDefault(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void askToSetAsDefault(
      const v8::FunctionCallbackInfo<v8::Value>& info);

  template <class T>
  using V8Persistent = v8::Persistent<T, v8::CopyablePersistentTraits<T>>;

  void CallQueryCallback(V8Persistent<v8::Function> callback, bool success);
  void CallAskCallback(V8Persistent<v8::Function> callback);

  v8::Persistent<v8::Context> v8_context_;
  YandexPromotionBindingsDelegate* delegate_;

  friend class base::RefCounted<YandexPromotionBindings>;
  ~YandexPromotionBindings();

  DISALLOW_COPY_AND_ASSIGN(YandexPromotionBindings);
};

}  // namespace opera

#endif  // CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_BINDINGS_H_
