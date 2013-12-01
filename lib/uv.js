var uvb = _uv_bindings;

// out module
var uv = {};

// enums
[
    // loop
    'UV_RUN_DEFAULT',
    'UV_RUN_ONCE',
    'UV_RUN_NOWAIT',

    // multicast
    'UV_LEAVE_GROUP',
    'UV_JOIN_GROUP'
].forEach(function(val) {
    uv[val] = uvb[val];
});

uv.version = function() {
    return uvb.version();
}

uv.version_string = function() {
    return uvb.version_string();
}

uv.strerror = function(errno) {
    return uvb.strerror(errno);
}

uv.err_name = function(errno) {
    return uvb.err_name(errno);
}

uv.default_loop = function() {
    return uvb.default_loop();
};

uv.now = function(loop) {
    return uvb.now(loop);
};

uv.run = function(loop, run_mode) {
    return uvb.run(loop, run_mode);
}

uv.buf_init = function(len) {
    return uvb.buf_init(len);
}

uv.fs_open = function(loop, path, flags, mode, cb) {
    return uvb.fs_open(loop, path, flags, mode, cb);
}

uv.fs_close = function(loop, fd, cb) {
    return uvb.fs_close(loop, fd, cb);
}

// @param buf should be a buffer returned from uv_buf_init
uv.fs_read = function(loop, fd, buf, offset, cb) {
    return uvb.fs_read(loop, fd, buf, offset, cb);
}

uv.fs_readdir = function(loop, path, flags, cb) {
    return uvb.fs_readdir(loop, path, flags, cb);
}

return uv;
