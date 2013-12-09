#pragma once

#include <assert.h>

namespace uvjs {
namespace detail {

// Callback holds a persistent function that can be invoked
// callback can also be asked to reset and free the function
class Callback {
public:
    Callback() {}

    ~Callback() {
        if (handle.IsEmpty()) {
            return;
        }

        handle.Dispose();
        handle.Reset();
    }

    // clears the persistent if we no longer need the callback
    void Reset() {
        handle.Reset();
    }

    void Reset(const v8::Local<v8::Value>& fn) {
        assert(fn->IsFunction());
        handle.Reset(v8::Isolate::GetCurrent(), v8::Local<v8::Function>::Cast(fn));
    }

    bool IsEmpty() {
        return handle.IsEmpty();
    }

    void Call(int argc, v8::Local<v8::Value> argv[]) {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);

        v8::Local<v8::Function> fn = v8::Local<v8::Function>::New(isolate, handle);

        v8::TryCatch try_catch;
        try_catch.SetVerbose(true);

        fn->Call(v8::Context::GetCurrent()->Global(), argc, argv);

        if (try_catch.HasCaught()) {
            isolate->ThrowException(try_catch.Exception());
        }
    }

private:
    v8::Persistent<v8::Function> handle;
};

} // namespace detail
} // namespace uvjs
