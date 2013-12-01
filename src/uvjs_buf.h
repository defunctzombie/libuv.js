#pragma once

#include <assert.h>
#include <stdlib.h>

#include <uv.h>

namespace uvjs {
namespace detail {

// helper to unwrap a v8::Value into a uv_buf_t*
// the value is expected to be Object type
// and have an internal field for the uv_buf_t*
static inline uv_buf_t* UnwrapBuf(v8::Handle<v8::Value> val) {

    assert(val->IsObject());

    v8::Handle<v8::Object> handle = val->ToObject();
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);

    return static_cast<uv_buf_t*>(handle->GetAlignedPointerFromInternalField(0));
}


// cleanup a char* created by a uv_buf_init
// we don't use object_wrap to be leaner
static void WeakUvBuf(v8::Isolate* isolate, v8::Persistent<v8::Object>* persistent,
        uv_buf_t* buf) {

    v8::HandleScope scope(isolate);
    assert(buf);

    if (persistent->IsEmpty()) {
        return;
    }

    assert(persistent->IsNearDeath());
    persistent->ClearWeak();
    persistent->Dispose();

    if (buf->base != NULL) {
        free(buf->base);
    }

    delete buf;
}

void buf_init(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 1);

    const int32_t len = args[0]->Int32Value();
    char* base = static_cast<char*>(malloc(len));
    assert(base);

    uv_buf_t buf = uv_buf_init(base, len);

    uv_buf_t* new_buf = new uv_buf_t();
    new_buf->base = buf.base;
    new_buf->len = buf.len;

    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    v8::Local<v8::Object> instance = obj->NewInstance();
    instance->SetAlignedPointerInInternalField(0, new_buf);

    v8::Persistent<v8::Object> persistent;
    persistent.Reset(v8::Isolate::GetCurrent(), instance);
    persistent.MakeWeak(new_buf, WeakUvBuf);
    persistent.MarkIndependent();

    args.GetReturnValue().Set(instance);
}

} // namespace detail
} // namespace uvjs
