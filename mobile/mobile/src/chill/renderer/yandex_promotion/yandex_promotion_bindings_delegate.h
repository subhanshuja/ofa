// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_BINDINGS_DELEGATE_H_
#define CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_BINDINGS_DELEGATE_H_

#include <string>

#include "base/callback.h"

namespace opera {

class YandexPromotionBindingsDelegate {
 public:
  using CanAskCallback = base::Callback<void(bool /* can_ask */)>;

  virtual void OnCanAskToSetAsDefault(std::string search_engine,
                                      const CanAskCallback& callback) {}
  virtual void OnAskToSetAsDefault(std::string search_engine,
                                   const base::Closure& callback) {}
};

}  // namespace opera

#endif  // CHILL_RENDERER_YANDEX_PROMOTION_YANDEX_PROMOTION_BINDINGS_DELEGATE_H_
