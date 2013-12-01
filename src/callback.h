#pragma once

namespace uvjs {
namespace detail {

template <class TypeName>
static v8::Local<TypeName> NanPersistentToLocal(const v8::Persistent<TypeName>& persistent) {
  static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();
  v8::HandleScope scope(nan_isolate);

  if (persistent.IsWeak()) {
    return v8::Local<TypeName>::New(nan_isolate, persistent);
  }

  return *reinterpret_cast<v8::Local<TypeName>*>(
        const_cast<v8::Persistent<TypeName>*>(&persistent));
}

class NanCallback {
  public:
    NanCallback(const v8::Local<v8::Function> &fn) {
      static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();
      v8::HandleScope scope(nan_isolate);

      v8::Local<v8::Object> obj = v8::Object::New();
      obj->Set(v8::String::NewSymbol("callback"), fn);
      handle.Reset(nan_isolate, obj);
    }

    ~NanCallback() {
      if (handle.IsEmpty()) return;
      handle.Dispose();
      handle.Clear();
    }

    v8::Local<v8::Function> GetFunction () {
      return NanPersistentToLocal(handle)->Get(v8::String::NewSymbol("callback"))
        .As<v8::Function>();
    }

    void Call(int argc, v8::Local<v8::Value> argv[]) {
      static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();
      v8::HandleScope scope(nan_isolate);

      v8::Local<v8::Function> callback = NanPersistentToLocal(handle)->
        Get(v8::String::NewSymbol("callback")).As<v8::Function>();

      v8::TryCatch try_catch;
      try_catch.SetVerbose(true);

      v8::Local<v8::Value> ret = callback->Call(v8::Context::GetCurrent()->Global(), argc, argv);

      if (try_catch.HasCaught()) {
        // TODO ??
      }
    }

  private:
    v8::Persistent<v8::Object> handle;
};

} // namespace detail
} // namespace uvjs
