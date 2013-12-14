#pragma once

#include <v8.h>

namespace uvjs {

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
    // return a void* after externalizing an array
    // should be able to handle an already externalized array
    virtual void* Externalized(v8::Local<v8::ArrayBuffer>& buffer) = 0;

    // given a raw pointer allocated with via Allocate(size_t len)
    // return a properly externalized wrapped AB
    virtual v8::Local<v8::ArrayBuffer> Externalize(void* buf, size_t bytes) = 0;
};

void SetArrayBufferAllocator(uvjs::ArrayBufferAllocator*);

// return a js object with uv functions
// this should be called within a HandleScope
//
// example of exposing on existing object via c++
// v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
// global->Set(v8::String::New("_uv_bindings"), uvjs::New());
//
// Then use lib/uv.js to access the bindings
//
v8::Handle<v8::ObjectTemplate> New();

} // namespace uvjs
