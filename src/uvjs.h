#pragma once

#include <v8.h>

namespace uvjs {

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
