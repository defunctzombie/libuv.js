#pragma once

#include <assert.h>
#include <v8.h>
#include <uv.h>

#include "stream_wrap.h"
#include "unwrap.h"
#include "throw.h"

namespace uvjs {
namespace detail {

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

    // TODO need to accept this info via arg
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

void tcp_init(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 1);

    uv_loop_t* loop = Unwrap<uv_loop_t>(args[0]);

    TcpWrap* wrap = new TcpWrap();

    int err = wrap->init(loop);
    if (err) {
        delete wrap;
        return UVThrow(err);
    }

    // timer object container
    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    TcpWrap::Mixin(obj);

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
