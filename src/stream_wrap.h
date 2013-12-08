#pragma once

#include <assert.h>
#include <v8.h>
#include <uv.h>

#include "handle_wrap.h"
#include "callback.h"

namespace uvjs {
namespace detail {

void After_Listen(uv_stream_t* server, int status);

template <typename T>
class StreamWrap : public HandleWrap<T> {
public:
    StreamWrap() : HandleWrap<T>() {}

    int listen(int backlog) {
        assert(this->_handle);
        this->Ref();
        return uv_listen(reinterpret_cast<uv_stream_t*>(this->_handle), backlog, After_Listen);
    }

    Callback& listen_callback() {
        return _listen_cb;
    }

    static void Stream_Listen(const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::HandleScope handle_scope(args.GetIsolate());

        assert(args.Length() == 2);
        assert(args[0]->IsInt32());

        StreamWrap<uv_stream_t>* wrap = Unwrap<StreamWrap<uv_stream_t> >(args.This());

        wrap->listen_callback().Reset(args[2]);

        const int err = wrap->listen(args[0]->Int32Value());
        args.GetReturnValue().Set(v8::Integer::New(err));
    }

    static void Mixin(v8::Handle<v8::ObjectTemplate> obj) {
        HandleWrap<T>::Mixin(obj);

        // stream specific things
        obj->Set(v8::String::NewSymbol("listen"), v8::FunctionTemplate::New(Stream_Listen));
    }

protected:
    Callback _listen_cb;

private:
};

void After_Listen(uv_stream_t* server, int status) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);

    assert(server->data);

    StreamWrap<uv_stream_t>* wrap = static_cast<StreamWrap<uv_stream_t> *>(server->data);

    v8::Local<v8::Value> instance = v8::Local<v8::Object>::New(isolate, wrap->persistent());

    wrap->Unref();

    if (!wrap->listen_callback().IsEmpty()) {
        const int argc = 2;
        v8::Local<v8::Value> argv[argc] = { instance , v8::Integer::New(status) };
        wrap->listen_callback().Call(argc, argv);
    }
}

} // namespace detail
} // namespace uvjs
