#pragma once

#include <assert.h>
#include <uv.h>

namespace uvjs {
namespace detail {

void version(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    args.GetReturnValue().Set(v8::Integer::New(uv_version()));
}

void version_string(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    args.GetReturnValue().Set(v8::String::New(uv_version_string()));
}

void strerror(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 1);
    assert(args[0]->IsInt32());

    args.GetReturnValue().Set(v8::String::New(uv_strerror(args[0]->Int32Value())));
}

void err_name(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 1);
    assert(args[0]->IsInt32());

    args.GetReturnValue().Set(v8::String::New(uv_err_name(args[0]->Int32Value())));
}

} // namespace detail
} // namespace uvjs
