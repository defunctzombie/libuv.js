#pragma once

#include <assert.h>
#include <uv.h>

#include "callback.h"

namespace uvjs {
namespace detail {

template <typename T>
static inline T* Unwrap(v8::Handle<v8::Value> val) {
    assert(val->IsObject());

    v8::Handle<v8::Object> handle = val->ToObject();
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);

    return static_cast<T*>(handle->GetAlignedPointerFromInternalField(0));
}

static inline uv_stream_t* UnwrapStream(v8::Handle<v8::Value> val) {

    assert(val->IsObject());

    v8::Handle<v8::Object> handle = val->ToObject();
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);

    return static_cast<uv_stream_t*>(handle->GetAlignedPointerFromInternalField(0));
}

static void WeakUvStream(v8::Isolate* isolate, v8::Persistent<v8::Object>* persistent,
        uv_tcp_t* stream) {

    v8::HandleScope scope(isolate);
    assert(stream);

    if (persistent->IsEmpty()) {
        return;
    }

    assert(persistent->IsNearDeath());
    persistent->ClearWeak();
    persistent->Dispose();

    delete stream;
}

void After_listen(uv_stream_t* server, int status) {
    static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(nan_isolate);

    assert(server->data);
    NanCallback* req_wrap = static_cast<NanCallback*>(server->data);

    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = { v8::Undefined() , v8::Undefined() };

    // wrap the server
    // wrap status
    req_wrap->Call(argc, argv);

    delete req_wrap;
}

void After_close(uv_handle_t* handle) {
    static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(nan_isolate);

    assert(handle->data);
    NanCallback* req_wrap = static_cast<NanCallback*>(handle->data);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { v8::Undefined() };

    req_wrap->Call(argc, argv);

    delete req_wrap;
}

void __stream_new(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    uv_tcp_t* new_stream = new uv_tcp_t;

    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    v8::Local<v8::Object> instance = obj->NewInstance();
    instance->SetAlignedPointerInInternalField(0, new_stream);

    v8::Persistent<v8::Object> persistent;
    persistent.Reset(v8::Isolate::GetCurrent(), instance);
    persistent.MakeWeak(new_stream, WeakUvStream);
    persistent.MarkIndependent();

    args.GetReturnValue().Set(instance);
}

void listen(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 3);
    assert(args[1]->IsInt32());

    // user needs to be able to make a uv_stream_t
    // this can be done via new uv.stream_t()
    // which will return a stream_t wrapper?

    uv_stream_t* stream = UnwrapStream(args[0]);

    stream->data = new NanCallback(v8::Local<v8::Function>::Cast(args[2]));

    const int res = uv_listen(stream, args[1]->Int32Value(), After_listen);

    if (res != 0) {
        delete stream->data;
        stream->data = NULL;
    }

    args.GetReturnValue().Set(v8::Integer::New(res));
}

void tcp_init(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 2);

    // user needs to be able to make a uv_stream_t
    // this can be done via new uv.stream_t()
    // which will return a stream_t wrapper?

    uv_loop_t* loop = UnwrapLoop(args[0]);
    uv_tcp_t* stream = Unwrap<uv_tcp_t>(args[1]);

    const int res = uv_tcp_init(loop, stream);

    assert(res == 0);

    args.GetReturnValue().Set(v8::Integer::New(res));
}

void close(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 2);

    // handle->data needs to point to the wrapper we have
    // and not each callback...?

    uv_handle_t* handle = Unwrap<uv_handle_t>(args[0]);

    // shit... we can't do this
    // well.. wtf do we do?

    handle->data = new NanCallback(v8::Local<v8::Function>::Cast(args[1]));

    uv_close(handle, After_close);
}

} // namespace detail
} // namespace uvjs
