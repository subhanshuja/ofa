// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "third_party/WebKit/public/web/WebKit.h"

#include "chill/renderer/yandex_promotion/exception_messages.h"
#include "chill/renderer/yandex_promotion/yandex_promotion_bindings.h"
#include "chill/renderer/yandex_promotion/yandex_promotion_bindings_delegate.h"

namespace {

std::string V8StringToUtf8String(v8::Local<v8::String> v8_string) {
  int buffer_size = v8_string->Utf8Length() + 1 /* '\0 '*/;
  std::unique_ptr<char[]> buffer(new char[buffer_size]);
  v8_string->WriteUtf8(buffer.get(), buffer_size);

  return std::string(buffer.get(), buffer_size);
}

bool CheckArgumentLength(v8::Isolate* isolate,
                         const char* method,
                         const char* type,
                         unsigned int expected,
                         unsigned int optional,
                         unsigned int provided) {
  if (provided > expected + optional) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(
        isolate, opera::ExceptionMessages::FailedToExecute(
                     method, type, opera::ExceptionMessages::TooManyArguments(
                                       expected + optional, provided))
                     .c_str())));

    return false;
  }

  if (provided < expected) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(
        isolate, opera::ExceptionMessages::FailedToExecute(
                     method, type, opera::ExceptionMessages::NotEnoughArguments(
                                       expected, provided))
                     .c_str())));
    return false;
  }

  return true;
}

template <class T>
bool IsType(v8::Local<v8::Value> value);

template <>
bool IsType<v8::String>(v8::Local<v8::Value> value) {
  return value->IsString();
}

template <>
bool IsType<v8::Function>(v8::Local<v8::Value> value) {
  return value->IsFunction();
}

template <class T>
std::string TypeString();

template <>
std::string TypeString<v8::String>() {
  return "String";
}

template <>
std::string TypeString<v8::Function>() {
  return "function";
}

template <class T>
v8::MaybeLocal<T> TypeCheckArgument(v8::Isolate* isolate,
                                    const char* method,
                                    const char* type,
                                    unsigned int argument_index,
                                    v8::Local<v8::Value> value) {
  if (IsType<T>(value)) {
    return v8::MaybeLocal<T>(value.As<T>());
  }

  isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(
      isolate,
      opera::ExceptionMessages::FailedToExecute(
          method, type, opera::ExceptionMessages::ArgumentNullOrIncorrectType(
                            argument_index, TypeString<T>()))
          .c_str())));

  return v8::MaybeLocal<T>();
}

opera::YandexPromotionBindings* BindingsFromValue(v8::Local<v8::Value> value) {
  DCHECK(!value.IsEmpty());
  DCHECK(value->IsExternal());

  return static_cast<opera::YandexPromotionBindings*>(
      v8::External::Cast(*value)->Value());
}

}  // namespace

namespace opera {

YandexPromotionBindings::YandexPromotionBindings(
    YandexPromotionBindingsDelegate* delegate,
    v8::Local<v8::Context> v8_context)
    : delegate_(delegate) {
  v8_context_.Reset(v8_context->GetIsolate(), v8_context);
  Inject(v8_context->GetIsolate());
}

YandexPromotionBindings::~YandexPromotionBindings() {}

void YandexPromotionBindings::Inject(v8::Isolate* isolate) {
  v8::Local<v8::Context> v8_context = v8_context_.Get(isolate);
  v8::Local<v8::FunctionTemplate> can_ask_template = v8::FunctionTemplate::New(
      isolate, &YandexPromotionBindings::canAskToSetAsDefault,
      v8::External::New(isolate, reinterpret_cast<void*>(this)));

  v8::Local<v8::FunctionTemplate> ask_template = v8::FunctionTemplate::New(
      isolate, &YandexPromotionBindings::askToSetAsDefault,
      v8::External::New(isolate, reinterpret_cast<void*>(this)));

  v8::Local<v8::Object> engine_object = v8::Object::New(isolate);
  engine_object->DefineOwnProperty(
      v8_context, v8::String::NewFromUtf8(isolate, "YANDEX"),
      v8::String::NewFromUtf8(isolate, "yandex"), v8::ReadOnly);

  // Create 'searchEnginesPrivate' API object
  v8::Local<v8::Object> private_api = v8::Object::New(isolate);
  private_api->DefineOwnProperty(v8_context,
                                 v8::String::NewFromUtf8(isolate, "Engine"),
                                 engine_object, v8::ReadOnly);
  private_api->DefineOwnProperty(
      v8_context, v8::String::NewFromUtf8(isolate, "canAskToSetAsDefault"),
      can_ask_template->GetFunction());
  private_api->DefineOwnProperty(
      v8_context, v8::String::NewFromUtf8(isolate, "askToSetAsDefault"),
      ask_template->GetFunction());

  // Wire up 'searchEnginesPrivate' with 'opr' and the 'window' object
  v8::Local<v8::Object> opr_object = v8::Object::New(isolate);
  opr_object->DefineOwnProperty(
      v8_context, v8::String::NewFromUtf8(isolate, "searchEnginesPrivate"),
      private_api, v8::ReadOnly);
  v8_context->Global()->DefineOwnProperty(
      v8_context, v8::String::NewFromUtf8(isolate, "opr"), opr_object);
}

void YandexPromotionBindings::CallQueryCallback(
    V8Persistent<v8::Function> callback,
    bool success) {
  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtask_scope(
      isolate, v8::MicrotasksScope::Type::kRunMicrotasks);

  v8::Local<v8::Value> args[] = {v8::Boolean::New(isolate, success)};
  callback.Get(isolate)->Call(v8_context_.Get(isolate),
                              v8_context_.Get(isolate)->Global(),
                              arraysize(args), args);
}

void YandexPromotionBindings::canAskToSetAsDefault(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  DCHECK_EQ(isolate, blink::mainThreadIsolate());

  if (!CheckArgumentLength(isolate, "canAskToSetAsDefault",
                           "searchEnginesPrivate", 2 /* expected */,
                           0 /* optional */, info.Length())) {
    return;
  }

  v8::MaybeLocal<v8::String> engine = TypeCheckArgument<v8::String>(
      isolate, "canAskToSetAsDefault", "searchEnginesPrivate",
      1 /* argument_index */, info[0]);
  if (engine.IsEmpty()) {
    return;
  }

  v8::MaybeLocal<v8::Function> callback = TypeCheckArgument<v8::Function>(
      isolate, "canAskToSetAsDefault", "searchEnginesPrivate",
      2 /* argument_index */, info[1]);
  if (callback.IsEmpty()) {
    return;
  }

  opera::YandexPromotionBindings* bindings = BindingsFromValue(info.Data());

  bindings->delegate_->OnCanAskToSetAsDefault(
      V8StringToUtf8String(engine.ToLocalChecked()),
      base::Bind(&YandexPromotionBindings::CallQueryCallback, bindings,
                 v8::Persistent<v8::Function,
                                v8::CopyablePersistentTraits<v8::Function>>(
                     info.GetIsolate(), callback.ToLocalChecked())));
}

void YandexPromotionBindings::CallAskCallback(
    V8Persistent<v8::Function> callback) {
  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtask_scope(
      isolate, v8::MicrotasksScope::Type::kRunMicrotasks);

  callback.Get(isolate)->Call(v8_context_.Get(isolate),
                              v8_context_.Get(isolate)->Global(), 0, nullptr);
}

void YandexPromotionBindings::askToSetAsDefault(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  DCHECK_EQ(isolate, blink::mainThreadIsolate());

  if (!CheckArgumentLength(isolate, "askToSetAsDefault", "searchEnginesPrivate",
                           1 /* expected */, 1 /* optional */, info.Length())) {
    return;
  }

  v8::MaybeLocal<v8::String> engine = TypeCheckArgument<v8::String>(
      isolate, "askToSetAsDefault", "searchEnginesPrivate",
      1 /* argument_index */, info[0]);
  if (engine.IsEmpty()) {
    return;
  }

  opera::YandexPromotionBindings* bindings = BindingsFromValue(info.Data());
  base::Closure callback;

  if (info.Length() == 2) {
    v8::MaybeLocal<v8::Function> maybe_callback =
        TypeCheckArgument<v8::Function>(isolate, "askToSetAsDefault",
                                        "searchEnginesPrivate",
                                        2 /* argument_index */, info[1]);
    if (maybe_callback.IsEmpty()) {
      return;
    }

    callback = base::Bind(
        &YandexPromotionBindings::CallAskCallback, bindings,
        V8Persistent<v8::Function>(isolate, maybe_callback.ToLocalChecked()));
  }

  bindings->delegate_->OnAskToSetAsDefault(
      V8StringToUtf8String(engine.ToLocalChecked()), callback);
}

}  // namespace opera
