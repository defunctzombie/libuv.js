#pragma once

#include <assert.h>
#include <uv.h>

#include "unwrap.h"

namespace uvjs {
namespace detail {

// cleanup a uv_loop_t* created in loop_new
// we don't use object_wrap to be leaner
static void WeakUvLoop(v8::Isolate* isolate, v8::Persistent<v8::Object>* persistent,
        uv_loop_t* loop) {

    v8::HandleScope scope(isolate);
    assert(loop);

    if (persistent->IsEmpty()) {
        return;
    }

    assert(persistent->IsNearDeath());
    persistent->ClearWeak();
    persistent->Dispose();

    uv_loop_delete(loop);
}

void loop_new(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    uv_loop_t* new_loop = uv_loop_new();

    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    v8::Local<v8::Object> instance = obj->NewInstance();
    instance->SetAlignedPointerInInternalField(0, new_loop);

    v8::Persistent<v8::Object> persistent;
    persistent.Reset(v8::Isolate::GetCurrent(), instance);
    persistent.MakeWeak(new_loop, WeakUvLoop);
    persistent.MarkIndependent();

    args.GetReturnValue().Set(instance);
}

void default_loop(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    v8::Handle<v8::ObjectTemplate> obj = v8::ObjectTemplate::New();
    obj->SetInternalFieldCount(1);

    v8::Local<v8::Object> obj_inst = obj->NewInstance();
    obj_inst->SetAlignedPointerInInternalField(0, uv_default_loop());

    args.GetReturnValue().Set(obj_inst);
}

void run(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args.Length() == 2);
    assert(args[1]->IsInt32());

    const int run_mode = args[1]->ToInteger()->Value();
    const int result = uv_run(Unwrap<uv_loop_t>(args[0]), static_cast<uv_run_mode>(run_mode));

    args.GetReturnValue().Set(v8::Integer::New(result));
}

void stop(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    assert(args.Length() == 1);
    uv_stop(Unwrap<uv_loop_t>(args[0]));
}

void update_time(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    assert(args.Length() == 1);
    uv_update_time(Unwrap<uv_loop_t>(args[0]));
}

void backend_fd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    assert(args.Length() == 1);
    const int fd = uv_backend_fd(Unwrap<uv_loop_t>(args[0]));
    args.GetReturnValue().Set(v8::Integer::New(fd));
}

void backend_timeout(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    assert(args.Length() == 1);
    const int timeout = uv_backend_timeout(Unwrap<uv_loop_t>(args[0]));
    args.GetReturnValue().Set(v8::Integer::New(timeout));
}

void now(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());
    assert(args.Length() == 1);

    // we don't have a good way to create 64bit numbers in js
    // so for now we will just cast this to double
    const uint64_t now = uv_now(Unwrap<uv_loop_t>(args[0]));
    args.GetReturnValue().Set(v8::Number::New(static_cast<double>(now)));
}

} // namespace detail
} // namespace uvjs
