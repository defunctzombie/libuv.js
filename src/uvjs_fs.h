#pragma once

#include <string.h>

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

    // deprecated
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

v8::Local<v8::Value> UVException(const uv_fs_t& res) {
  static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();

  const int32_t errorno = res.result;
  const char* msg = uv_strerror(errorno);

  v8::Local<v8::String> estring = v8::String::NewFromUtf8(nan_isolate, uv_err_name(errorno));
  v8::Local<v8::String> message = v8::String::NewFromUtf8(nan_isolate, msg);
  v8::Local<v8::String> cons1 =
      v8::String::Concat(estring, v8::String::NewFromUtf8(nan_isolate, ", "));
  v8::Local<v8::String> cons2 = v8::String::Concat(cons1, message);

  v8::Local<v8::Value> err = v8::Exception::Error(cons2);

  v8::Local<v8::Object> obj = err->ToObject();
  obj->Set(v8::String::New("errno"), v8::Integer::New(errno, nan_isolate));
  obj->Set(v8::String::New("code"), estring);

  return err;
}

v8::Local<v8::Value> UVException(const int errorno, const char *msg) {
  static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();

  if (!msg || !msg[0]) {
    msg = uv_strerror(errorno);
  }

  v8::Local<v8::String> estring = v8::String::NewFromUtf8(nan_isolate, uv_err_name(errorno));
  v8::Local<v8::String> message = v8::String::NewFromUtf8(nan_isolate, msg);
  v8::Local<v8::String> cons1 =
      v8::String::Concat(estring, v8::String::NewFromUtf8(nan_isolate, ", "));
  v8::Local<v8::String> cons2 = v8::String::Concat(cons1, message);

  v8::Local<v8::Value> e = v8::Exception::Error(cons2);

  v8::Local<v8::Object> obj = e->ToObject();
  // TODO(piscisaureus) errno should probably go
  obj->Set(v8::String::New("errno"), v8::Integer::New(errorno, nan_isolate));
  obj->Set(v8::String::New("code"), estring);

  return e;
}

inline static void ThrowUVException(const uv_fs_t& req) {
  v8::ThrowException(UVException(req));
}

v8::Local<v8::Object> BuildStatsObject(const uv_stat_t* s) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Object> stats = v8::Object::New();
  //Local<Object> stats = env->stats_constructor_function()->NewInstance();
  if (stats.IsEmpty()) {
    return v8::Local<v8::Object>();
  }

  // The code below is very nasty-looking but it prevents a segmentation fault
  // when people run JS code like the snippet below. It's apparently more
  // common than you would expect, several people have reported this crash...
  //
  //   function crash() {
  //     fs.statSync('.');
  //     crash();
  //   }
  //
  // We need to check the return value of Integer::New() and Date::New()
  // and make sure that we bail out when V8 returns an empty handle.
#define X(name)                                                               \
  {                                                                           \
    v8::Local<v8::Value> val = v8::Integer::New(s->st_##name, isolate);       \
    if (val.IsEmpty())                                                        \
      return v8::Local<v8::Object>();                                         \
    stats->Set(v8::String::New("##name"), val);                                  \
  }
  X(dev)
  X(mode)
  X(nlink)
  X(uid)
  X(gid)
  X(rdev)
# if defined(__POSIX__)
  X(blksize)
# endif
#undef X

#define X(name)                                                               \
  {                                                                           \
    v8::Local<v8::Value> val = v8::Number::New(static_cast<double>(s->st_##name));\
    if (val.IsEmpty())                                                        \
      return v8::Local<v8::Object>();                                         \
    stats->Set(v8::String::New("##name"), val);                                  \
  }
  X(ino)
  X(size)
# if defined(__POSIX__)
  X(blocks)
# endif
#undef X

#define X(name, rec)                                                          \
  {                                                                           \
    double msecs = static_cast<double>(s->st_##rec.tv_sec) * 1000;            \
    msecs += static_cast<double>(s->st_##rec.tv_nsec / 1000000);              \
    v8::Local<v8::Value> val = v8::Date::New(msecs);                          \
    if (val.IsEmpty())                                                        \
      return v8::Local<v8::Object>();                                         \
    stats->Set(v8::String::New("##name"), val);                                 \
  }
  X(atime, atim)
  X(mtime, mtim)
  X(ctime, ctim)
  X(birthtime, birthtim)
#undef X

  return handle_scope.Close(stats);
}

static void After(uv_fs_t* req) {
  static v8::Isolate* nan_isolate = v8::Isolate::GetCurrent();
  v8::HandleScope scope(nan_isolate);

  assert(req->data);
  NanCallback* req_wrap = static_cast<NanCallback*>(req->data);

  // there is always at least one argument. "error"
  int argc = 1;

  // Allocate space for two args. We may only use one depending on the case.
  // (Feel free to increase this if you need more)
  v8::Local<v8::Value> argv[2];

  if (req->result < 0) {
    // If the request doesn't have a path parameter set.
    // TODO

    if (req->path == NULL) {
      argv[0] = UVException(req->result, NULL);
    } else {
      argv[0] = UVException(req->result, NULL);
    }
  } else {
    // error value is empty or null for non-error.
    argv[0] = Null(nan_isolate);

    // All have at least two args now.
    argc = 2;

    switch (req->fs_type) {
      // These all have no data to pass.
      case UV_FS_CLOSE:
      case UV_FS_RENAME:
      case UV_FS_UNLINK:
      case UV_FS_RMDIR:
      case UV_FS_MKDIR:
      case UV_FS_FTRUNCATE:
      case UV_FS_FSYNC:
      case UV_FS_FDATASYNC:
      case UV_FS_LINK:
      case UV_FS_SYMLINK:
      case UV_FS_CHMOD:
      case UV_FS_FCHMOD:
      case UV_FS_CHOWN:
      case UV_FS_FCHOWN:
        // These, however, don't.
        argc = 1;
        break;

      case UV_FS_UTIME:
      case UV_FS_FUTIME:
        argc = 0;
        break;

      case UV_FS_OPEN:
        argv[1] = v8::Integer::New(req->result, nan_isolate);
        break;

      case UV_FS_WRITE:
        argv[1] = v8::Integer::New(req->result, nan_isolate);
        break;

      case UV_FS_STAT:
      case UV_FS_LSTAT:
      case UV_FS_FSTAT:
        argv[1] = BuildStatsObject(static_cast<const uv_stat_t*>(req->ptr));
        break;

      case UV_FS_READLINK:
        argv[1] = v8::String::NewFromUtf8(nan_isolate,
                                      static_cast<const char*>(req->ptr));
        break;

      case UV_FS_READ:
        // Buffer interface
        argv[1] = v8::Integer::New(req->result, nan_isolate);
        break;

      case UV_FS_READDIR:
        {
          char *namebuf = static_cast<char*>(req->ptr);
          int nnames = req->result;

          v8::Local<v8::Array> names = v8::Array::New(nnames);

          for (int i = 0; i < nnames; i++) {
            v8::Local<v8::String> name = v8::String::NewFromUtf8(nan_isolate, namebuf);
            names->Set(i, name);
#ifndef NDEBUG
            namebuf += strlen(namebuf);
            assert(*namebuf == '\0');
            namebuf += 1;
#else
            namebuf += strlen(namebuf) + 1;
#endif
          }

          argv[1] = names;
        }
        break;

      default:
        assert(0 && "Unhandled eio response");
    }
  }

  req_wrap->Call(argc, argv);

  uv_fs_req_cleanup(req);
  delete req_wrap;
}

void fs_readdir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args[1]->IsString());
    assert(args[2]->IsInt32());

    uv_loop_t* loop = UnwrapLoop(args[0]);
    v8::String::Utf8Value path(args[1]);
    const int flags = args[2]->Int32Value();

    // async
    if (args[3]->IsFunction()) {

      uv_fs_t* req = new uv_fs_t;

      NanCallback* cb = new NanCallback(v8::Local<v8::Function>::Cast(args[3]));
      req->data = cb;

      const int err = uv_fs_readdir(loop, req, *path, flags, After);
      if (err < 0) {
        req->result = err;
        req->path = NULL;
        After(req);
      }

      args.GetReturnValue().Set(v8::Integer::New(err));
      return;
    }

    // SYNC

    uv_fs_t req;
    uv_fs_cb cb = NULL;

    const int err = uv_fs_readdir(loop, &req, *path, flags, cb);
    if (err < 0) {
      req.result = err;
      return ThrowUVException(req);
    }

    assert(req.result >= 0);
    char* namebuf = static_cast<char*>(req.ptr);
    uint32_t nnames = req.result;
    v8::Local<v8::Array> names = v8::Array::New(nnames);

    for (uint32_t i = 0; i < nnames; ++i) {
      names->Set(i, v8::String::NewFromUtf8(args.GetIsolate(), namebuf));
#ifndef NDEBUG
      namebuf += strlen(namebuf);
      assert(*namebuf == '\0');
      namebuf += 1;
#else
      namebuf += strlen(namebuf) + 1;
#endif
    }

    uv_fs_req_cleanup(&req);
    args.GetReturnValue().Set(names);
}

void fs_open(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args[1]->IsString());
    assert(args[2]->IsInt32());
    assert(args[3]->IsInt32());

    uv_loop_t* loop = UnwrapLoop(args[0]);
    v8::String::Utf8Value path(args[1]);
    const int flags = args[2]->Int32Value();
    const int mode = args[3]->Int32Value();

    // async
    if (args[4]->IsFunction()) {
      uv_fs_t* req = new uv_fs_t;

      // wrap callback to make it persistent
      NanCallback* cb = new NanCallback(v8::Local<v8::Function>::Cast(args[4]));
      req->data = cb;

      const int err = uv_fs_open(loop, req, *path, flags, mode, After);
      if (err < 0) {
        req->result = err;
        req->path = NULL;
        After(req);
      }

      args.GetReturnValue().Set(v8::Integer::New(err));

      return;
    }

    // SYNC

    uv_fs_t req;

    const int err = uv_fs_open(loop, &req, *path, flags, mode, NULL);

    if (err < 0) {
      req.result = err;
      return ThrowUVException(req);
    }

    assert(req.result >= 0);

    uv_fs_req_cleanup(&req);
    args.GetReturnValue().Set(v8::Integer::New(req.result));
}

void fs_close(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args[1]->IsInt32());

    uv_loop_t* loop = UnwrapLoop(args[0]);
    const int32_t fd = args[1]->Int32Value();

    // async
    if (args[2]->IsFunction()) {

      uv_fs_t* req = new uv_fs_t;

      // wrap callback to make it persistent
      NanCallback* cb = new NanCallback(v8::Local<v8::Function>::Cast(args[2]));
      req->data = cb;

      const int err = uv_fs_close(loop, req, fd, After);
      if (err < 0) {
        req->result = err;
        req->path = NULL;
        After(req);
      }

      args.GetReturnValue().Set(v8::Integer::New(err));

      return;
    }

    // SYNC

    uv_fs_t req;

    const int err = uv_fs_close(loop, &req, fd, NULL);
    if (err < 0) {
      req.result = err;
      return ThrowUVException(req);
    }

    assert(req.result >= 0);

    uv_fs_req_cleanup(&req);
    args.GetReturnValue().Set(v8::Integer::New(req.result));
}

void fs_read(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::HandleScope handle_scope(args.GetIsolate());

    assert(args[1]->IsInt32());
    assert(args[3]->IsInt32());

    uv_loop_t* loop = UnwrapLoop(args[0]);
    const int fd = args[1]->Int32Value();
    uv_buf_t* buf = UnwrapBuf(args[2]);
    const int offset = args[3]->Int32Value();

    // async
    if (args[4]->IsFunction()) {

      uv_fs_t* req = new uv_fs_t;

      // wrap callback to make it persistent
      NanCallback* cb = new NanCallback(v8::Local<v8::Function>::Cast(args[4]));
      req->data = cb;

      const int err = uv_fs_read(loop, req, fd, buf->base, buf->len, offset, After);
      if (err < 0) {
        req->result = err;
        req->path = NULL;
        After(req);
      }

      args.GetReturnValue().Set(v8::Integer::New(err));

      return;
    }

    // SYNC

    uv_fs_t req;

    const int err = uv_fs_read(loop, &req, fd, buf->base, buf->len, offset, NULL);
    if (err < 0) {
      req.result = err;
      return ThrowUVException(req);
    }

    assert(req.result >= 0);

    uv_fs_req_cleanup(&req);
    args.GetReturnValue().Set(v8::Integer::New(req.result));
}

} // namespace detail
} // namespace uvjs
