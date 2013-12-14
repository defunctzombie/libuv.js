#pragma once

#include "uvjs.h"

namespace uvjs {
namespace detail {

// the array buffer allocator should be set by the embedder
// prior to any use of the uvjs bindings
uvjs::ArrayBufferAllocator* allocator = 0;

} // namespace detail
} // namespace uvjs
