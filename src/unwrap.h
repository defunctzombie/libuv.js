#pragma once

#include <assert.h>
#include <v8.h>

namespace uvjs {
namespace detail {

template <typename T>
inline T* Unwrap(v8::Handle<v8::Value> val) {
    assert(val->IsObject());

    v8::Handle<v8::Object> handle = val->ToObject();
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);

    return static_cast<T*>(handle->GetAlignedPointerFromInternalField(0));
}

} // namespace detail
} // namespace uvjs
