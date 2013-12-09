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
        assert(err == 0);

        struct sockaddr_in* sock = reinterpret_cast<struct sockaddr_in*>(&sockname);

        //uv_ip4_name to get string for IP

        printf("port %d\n", htons(sock->sin_port));
        return err;
    }

    int accept(TcpWrap* client) {
        return uv_accept(reinterpret_cast<uv_stream_t*>(_handle),
                reinterpret_cast<uv_stream_t*>(client->_handle));
    }

    static void Tcp_Accept(const v8::FunctionCallbackInfo<v8::Value>& args);

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
    assert(args[0]->IsObject());

    TcpWrap* wrap = Unwrap<TcpWrap>(args.This());

    // read fields from object into addr struct

    // result addr struct
    struct sockaddr_in addr;

    // process object art into sockaddr struct
    {
        // process
        v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(args[0]);
        v8::Local<v8::Value> port_val = obj->Get(v8::String::NewSymbol("port"));
        v8::Local<v8::Value> ip_val = obj->Get(v8::String::NewSymbol("address"));
        v8::Local<v8::Value> family_val = obj->Get(v8::String::NewSymbol("family"));

        assert(port_val->IsInt32());
        assert(ip_val->IsString());
        assert(family_val->IsString());

        const char* ip = *(v8::String::Utf8Value(ip_val));
        const int port = port_val->Int32Value();

        printf("%s %d\n", ip, port);

        assert(uv_ip4_addr(ip, port, &addr) == 0);
    }

    const int err = wrap->bind(reinterpret_cast<sockaddr*>(&addr));
    args.GetReturnValue().Set(v8::Integer::New(err));
}

void Tcp_Getsockname(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    TcpWrap* wrap = Unwrap<TcpWrap>(args.This());
    wrap->getsockname();

    args.GetReturnValue().Set(v8::Integer::New(0));
}

void TcpWrap::Tcp_Accept(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    TcpWrap* wrap = Unwrap<TcpWrap>(args.This());

    // new tcp wrapper for the client connect we accepted
    TcpWrap* client = new TcpWrap();

    // TODO(shtylman) interesting, can we accept into a different loop?
    int err = client->init(wrap->_handle->loop);
    if (err) {
        delete client;
        return UVThrow(err);
    }

    // create new object for client
    err = wrap->accept(client);
    if (err) {
        delete client;
        return UVThrow(err);
    }

    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    TcpWrap::Mixin(obj);

    // tcp stuff
    obj->Set(v8::String::NewSymbol("connect"), v8::FunctionTemplate::New(Tcp_Connect));
    obj->Set(v8::String::NewSymbol("bind"), v8::FunctionTemplate::New(Tcp_Bind));
    obj->Set(v8::String::NewSymbol("getsockname"), v8::FunctionTemplate::New(Tcp_Getsockname));

    v8::Local<v8::Object> instance = obj->NewInstance();
    client->Wrap(instance);

    args.GetReturnValue().Set(instance);
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

    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    TcpWrap::Mixin(obj);

    // tcp stuff
    obj->Set(v8::String::NewSymbol("connect"), v8::FunctionTemplate::New(Tcp_Connect));
    obj->Set(v8::String::NewSymbol("bind"), v8::FunctionTemplate::New(Tcp_Bind));
    obj->Set(v8::String::NewSymbol("getsockname"), v8::FunctionTemplate::New(Tcp_Getsockname));

    // technically a stream function, but we need to call uv_tcp_init on new handle
    obj->Set(v8::String::NewSymbol("accept"), v8::FunctionTemplate::New(TcpWrap::Tcp_Accept));

    v8::Local<v8::Object> instance = obj->NewInstance();
    wrap->Wrap(instance);

    args.GetReturnValue().Set(instance);
}

} // namespace detail
} // namespace uvjs
