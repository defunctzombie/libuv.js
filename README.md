# libuv.js [![Build Status](https://travis-ci.org/defunctzombie/libuv.js.png?branch=master)](https://travis-ci.org/defunctzombie/libuv.js)

v8 bindings for libuv. Mad science and experimentation.

If you don't know anything about c++ or v8 bindings this module is not for you.

## overview

Read the following files:

* src/uvjs.h
* lib/uv.js
* test/index.js

## run tests

```shell
make test
```

## why?

No opinions. No module system. No extra API. Just bindings to libuv for javascript. Build your own node.js like environment and make your own API sugar.

## documentation

There is none.

## I'm scared

You should be.

## implemented

* version()
* version_string()
* strerror(errno)
* err_name(errno)
* default_loop()
* now(loop)
* run(loop, run_mode)
* stop(loop)
* fs_open(loop, path, flags, mode, cb)
* fs_close(loop, fd, cb)
* fs_read(loop, fd, buf, offset, cb)
* fs_readdir(loop, path, flags, cb)
* tcp_init(loop)
* timer_imit(loop)

## credits

Some of the code is copied from the node.js, libuv, v8, and [nan](https://github.com/rvagg/nan) projects. Some of this will change as this project is cleaned up and made more mature.

