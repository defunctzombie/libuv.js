#include <v8.h>

#include "uvjs.h"
#include "uvjs_misc.h"
#include "uvjs_loop.h"
#include "uvjs_stream.h"
#include "uvjs_timer.h"
#include "uvjs_fs.h"

namespace uvjs {

v8::Handle<v8::ObjectTemplate> New() {

    v8::Handle<v8::ObjectTemplate> uv = v8::ObjectTemplate::New();

    // functions
#define PROP(str) uv->Set(v8::String::New(#str), v8::FunctionTemplate::New(uvjs::detail::str));

    // misc
    PROP(version);
    PROP(version_string);
    PROP(strerror);
    PROP(err_name);

    // loop
    PROP(loop_new);
    // uv_loop_delete does not exist, we let js manage loop lifetime
    PROP(default_loop);
    PROP(run);
    PROP(stop);
    PROP(update_time);
    PROP(backend_fd);
    PROP(backend_timeout);
    PROP(now);

    // timers
    PROP(timer_init);

    // streams
    PROP(tcp_init);

    // fs
    PROP(fs_open);
    PROP(fs_close);
    PROP(fs_read);
    PROP(fs_readdir);

#undef PROP

    // enums
#define ENUM(str) uv->Set(v8::String::New(#str), v8::Integer::New(str));

    ENUM(UV_RUN_DEFAULT);
    ENUM(UV_RUN_ONCE);
    ENUM(UV_RUN_NOWAIT);

    ENUM(UV_LEAVE_GROUP);
    ENUM(UV_JOIN_GROUP);

#undef ENUM

    return uv;
}

} // namespace uvjs

