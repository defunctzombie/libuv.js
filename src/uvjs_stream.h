#pragma once

#include <assert.h>
#include <uv.h>

#include "handle_wrap.h"
#include "unwrap.h"

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

protected:
    Callback _listen_cb;

private:
};

class TcpWrap : public StreamWrap<uv_tcp_t> {
public:
    TcpWrap() : StreamWrap<uv_tcp_t>() {}

    int init(uv_loop_t* loop) {
        return uv_tcp_init(loop, _handle);
    }

    int bind(const struct sockaddr* addr) {
        return uv_tcp_bind(_handle, addr);
    }

    // TODO get socket name
    // return socket struct wrapper/object access
    int getsockname() {
        struct sockaddr sockname;
        int namelen = sizeof sockname;
        const int err = uv_tcp_getsockname(_handle, &sockname, &namelen);

        struct sockaddr_in* sock = reinterpret_cast<struct sockaddr_in*>(&sockname);

        printf("port %d\n", sock->sin_port);
        return err;
    }

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

void Stream_Listen(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 2);
    assert(args[0]->IsInt32());

    StreamWrap<uv_stream_t>* wrap = Unwrap<StreamWrap<uv_stream_t> >(args.This());

    wrap->listen_callback().Reset(args[2]);

    const int err = wrap->listen(args[0]->Int32Value());
    args.GetReturnValue().Set(v8::Integer::New(err));
}

void Tcp_Connect(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    assert(args.Length() == 3);
    //TcpWrap* wrap = Unwrap<TcpWrap>(args.This());
}

void Tcp_Bind(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 1);

    TcpWrap* wrap = Unwrap<TcpWrap>(args.This());

    struct sockaddr_in addr;

    assert(uv_ip4_addr("0.0.0.0", 0, &addr) == 0);

    const int err = wrap->bind(reinterpret_cast<sockaddr*>(&addr));
    assert(err == 0);
    args.GetReturnValue().Set(v8::Integer::New(err));
}

void Tcp_Getsockname(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    TcpWrap* wrap = Unwrap<TcpWrap>(args.This());
    wrap->getsockname();

    args.GetReturnValue().Set(v8::Integer::New(0));
}

void Handle_Close(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 1);
    assert(args[0]->IsFunction());

    HandleWrap<uv_handle_t>* wrap = Unwrap<HandleWrap<uv_handle_t> >(args.This());

    wrap->close_callback().Reset(args[0]);
    wrap->close();
}

void tcp_init(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 1);

    uv_loop_t* loop = Unwrap<uv_loop_t>(args[0]);

    TcpWrap* wrap = new TcpWrap();

    int err = wrap->init(loop);
    assert(err == 0); // TODO uv throw

    // timer object container
    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    // TODO better mixin of methods from base classes

    // handle stuff
    obj->Set(v8::String::NewSymbol("close"), v8::FunctionTemplate::New(Handle_Close));

    // stream stuff
    obj->Set(v8::String::NewSymbol("listen"), v8::FunctionTemplate::New(Stream_Listen));

    // tcp stuff
    obj->Set(v8::String::NewSymbol("connect"), v8::FunctionTemplate::New(Tcp_Connect));
    obj->Set(v8::String::NewSymbol("bind"), v8::FunctionTemplate::New(Tcp_Bind));
    obj->Set(v8::String::NewSymbol("getsockname"), v8::FunctionTemplate::New(Tcp_Getsockname));

    v8::Local<v8::Object> instance = obj->NewInstance();
    wrap->Wrap(instance);

    args.GetReturnValue().Set(instance);
}

} // namespace detail
} // namespace uvjs
