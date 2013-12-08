#pragma once

#include <assert.h>
#include <v8.h>

namespace uvjs {
namespace detail {

// trigger a ThrowException on the current isolate
// should be called within a handle scope
inline void UVThrow(int errorno) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();

    v8::Local<v8::String> code = v8::String::NewFromUtf8(isolate, uv_err_name(errorno));
    v8::Local<v8::String> message = v8::String::NewFromUtf8(isolate, uv_strerror(errorno));

    v8::Local<v8::Value> err = v8::Exception::Error(message);

    v8::Local<v8::Object> obj = err->ToObject();
    obj->Set(v8::String::NewSymbol("errno"), v8::Integer::New(errorno));
    obj->Set(v8::String::NewSymbol("code"), code);

    isolate->ThrowException(err);
}

} // namespace detail
} // namespace uvjs
