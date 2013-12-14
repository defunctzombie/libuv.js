#pragma once

#include <assert.h>
#include <v8.h>
#include <uv.h>

#include "stream_wrap.h"
#include "unwrap.h"
#include "throw.h"

namespace uvjs {
namespace detail {

class TtyWrap : public StreamWrap<uv_tty_t> {
public:
    TtyWrap() : StreamWrap<uv_tty_t>() {}

    int init(uv_loop_t* loop, uv_file fd, int readable) {
        return uv_tty_init(loop, _handle, fd, readable);
    }
private:
};

void tty_init(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 3);
    assert(args[1]->IsInt32());
    assert(args[2]->IsInt32());

    uv_loop_t* loop = Unwrap<uv_loop_t>(args[0]);

    TtyWrap* wrap = new TtyWrap();

    uv_file fd = args[1]->Int32Value();
    const int readable = args[2]->Int32Value();

    int err = wrap->init(loop, fd, readable);
    if (err) {
        delete wrap;
        return UVThrow(err);
    }

    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    TtyWrap::Mixin(obj);

    // tty stuff
    //obj->Set(v8::String::NewSymbol("set_mode"), v8::FunctionTemplate::New(Tcp_Connect));
    //obj->Set(v8::String::NewSymbol("reset_mode"), v8::FunctionTemplate::New(Tcp_Bind));
    //obj->Set(v8::String::NewSymbol("get_winsize"), v8::FunctionTemplate::New(Tcp_Getsockname));

    v8::Local<v8::Object> instance = obj->NewInstance();
    wrap->Wrap(instance);

    args.GetReturnValue().Set(instance);
}

} // namespace detail
} // namespace uvjs
