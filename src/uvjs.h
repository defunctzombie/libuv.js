#pragma once

#include <v8.h>

namespace uvjs {

// The ArrayBufferAllocator is provides an interface to externalized array buffers.
// Once an ArrayBuffer has been externalized, the stupid v8 api no longer allows
// us to get the externalized data since it decides it no longer wants to store it
// even tho storing it with the lifetime of the ArrayBuffer object would make sense.
//
// In addition to the default requred v8 Allocator methods, th embedder must
// implement two more methods to provide consistent access to externalized array buffers.
// The implementation is left up to the embedder but typically will involve storing
// some sort of Watchdog class pointer in one of the internal fields of the ArrayBuffer
// so that the externalized data pointer can be accessed again when needed.
//
class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
    // return the void* for the ArrayBuffer contents
    // If the array buffer has not been externalized, externalize it and make
    // sure to free the memory once collected
    //
    // If the array buffer has been externalized, return the content*
    // typically by accessing some internal field where you stored it
    virtual void* Externalized(v8::Local<v8::ArrayBuffer>& buffer) = 0;

    // create a new ArrayBuffer that is already externalized with the given
    // bytes which have been allocated with the Allocate(size_t len) method of
    // the v8 Allocator baseclass
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
