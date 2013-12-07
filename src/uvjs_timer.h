#pragma once

#include <assert.h>
#include <uv.h>

#include "handle_wrap.h"
#include "callback.h"

namespace uvjs {
namespace detail {

void After_timer(uv_timer_t* handle, int status);

// timer wrap manages the lifetime of a uv_timer_t handle
// we have to be smart about this because the handle and timer callback function
// can outlive the user's desire to hold the handle
class TimerWrap : public HandleWrap<uv_timer_t> {
public:
    TimerWrap() : HandleWrap<uv_timer_t>(), _cb(0) {}

    ~TimerWrap() {
        if (_cb) {
            delete _cb;
        }
    }

    int init(uv_loop_t* loop) {
        return uv_timer_init(loop, _handle);
    }

    int stop() {
        const int err = uv_timer_stop(_handle);

        // free up the callback, and unref since we no longer
        // have to live for the callback
        if (_cb) {
            delete _cb;
            _cb = NULL;
            this->Unref();
        }

        return err;
    }

    int again() {
        return uv_timer_again(_handle);
    }

    int start(const int duration, const int repeat) {
        return uv_timer_start(_handle, After_timer, duration, repeat);
    }

    void set_callback(const v8::Local<v8::Function>& fn) {
        assert(!_cb);
        _cb = new NanCallback(fn);
    }

    void Call(int argc, v8::Local<v8::Value> argv[]) {
        _cb->Call(argc, argv);
    }

private:
    NanCallback* _cb;
};

// timer has fired
void After_timer(uv_timer_t* handle, int status) {
    static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(nan_isolate);

    assert(handle->data);

    TimerWrap* wrap = static_cast<TimerWrap*>(handle->data);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { v8::Integer::New(status) };

    // invoke timer callback
    wrap->Call(argc, argv);

    const int active = uv_is_active(reinterpret_cast<uv_handle_t*>(handle));
    if (active > 0) {
        // don't unref because timer will fire again
        return;
    }

    // stop an inactive timer so the callback can be freed?
    wrap->stop();
}

void Timer_Stop(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    TimerWrap* wrap = HandleWrap<uv_timer_t>::Unwrap<TimerWrap>(args.This());
    const int res = wrap->stop();
    assert(res == 0);

    args.GetReturnValue().Set(v8::Integer::New(res));
}

void Timer_Start(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 3);
    assert(args[0]->IsInt32());
    assert(args[1]->IsInt32());
    assert(args[2]->IsFunction());

    TimerWrap* wrap = HandleWrap<uv_timer_t>::Unwrap<TimerWrap>(args.This());

    const int duration = args[0]->Int32Value();
    const int repeat = args[1]->Int32Value();

    wrap->set_callback(v8::Local<v8::Function>::Cast(args[2]));

    const int err = wrap->start(duration, repeat);

    if (!err) {
        // we bump the timer wrapper so it is not cleaned up immediately
        // if the user doesn't hold on to the returned instance
        wrap->Ref();
    }

    args.GetReturnValue().Set(v8::Integer::New(err));
}

void Timer_Again(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    TimerWrap* wrap = HandleWrap<uv_timer_t>::Unwrap<TimerWrap>(args.This());
    const int res = wrap->again();
    assert(res == 0);

    args.GetReturnValue().Set(v8::Integer::New(res));
}


void timer_init(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 1);

    uv_loop_t* loop = Unwrap<uv_loop_t>(args[0]);

    TimerWrap* wrap = new TimerWrap();

    int err = wrap->init(loop);
    assert(err == 0); // TODO uv throw

    // timer object container
    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);
    obj->Set(v8::String::NewSymbol("stop"), v8::FunctionTemplate::New(Timer_Stop));
    obj->Set(v8::String::NewSymbol("start"), v8::FunctionTemplate::New(Timer_Start));
    obj->Set(v8::String::NewSymbol("again"), v8::FunctionTemplate::New(Timer_Again));
    //obj->Set(v8::String::NewSymbol("set_repeat"), v8::FunctionTemplate::New(Timer_Stop));
    //obj->Set(v8::String::NewSymbol("get_repeat"), v8::FunctionTemplate::New(Timer_Stop));

    v8::Local<v8::Object> instance = obj->NewInstance();
    wrap->Wrap(instance);

    args.GetReturnValue().Set(instance);
}

} // namespace detail
} // namespace uvjs
