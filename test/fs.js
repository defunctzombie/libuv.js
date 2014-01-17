var test = require('./support/test');
var assert = require('./support/assert');
var after = require('./support/after');
var uv = require('./support/uv');
var StringView = require('./support/StringView');

var default_loop = uv.default_loop();

var test_fs_fn = function(fn) {
    var args = Array.prototype.slice.call(arguments, 1);

    // remove callback for sync test
    var cb = args.pop();

    try {
        // sync functions need null callback
        args.push(null);
        cb(null, fn.apply(this, args))
    } catch (err) {
        cb(err);
    }

    args.pop(); // remove null callback

    args.push(cb);
    var res = fn.apply(this, args);
    assert(res === 0);
}

function mode_num(mode) {
    return parseInt(mode, 8);
}

test('fs_open - ENOENT', function(done) {
    done = after(2, done);

    test_fs_fn(uv.fs_open, default_loop, './test/support/fs/foo2.txt', 0, mode_num('0666'), function(err, fd) {
        assert(err);
        assert(err.code === 'ENOENT');
        assert(err.message === 'ENOENT, no such file or directory');
        done();
    });
});

test('fs_open - success - sync', function() {
    var fd = uv.fs_open(default_loop, './test/support/fs/foo.txt', 0, mode_num('0666'), null);
    assert(fd > 0, 'fd > 0');

    var res = uv.fs_close(default_loop, fd, null);
    assert(res === 0, 'close should work');
});

test('fs_open - success - async', function(done) {
    var res = uv.fs_open(default_loop, './test/support/fs/foo.txt', 0, mode_num('0666'), function(err, fd) {
        assert.ifError(err);
        assert(fd > 0, 'fd > 0');

        uv.fs_close(default_loop, fd, function(err) {
            assert.ifError(err);
            done();
        });
    });

    assert(res === 0);
});

test('fs_close - fail sync', function() {
    try {
        var res = uv.fs_close(default_loop, -1, null);
    } catch (err) {
        assert(err.code === 'EBADF', 'should be EBADF');
    } finally {
        assert(res !== 0);
    }
});

test('fs_read', function() {
    var buf = new ArrayBuffer(1024);

    var fd = uv.fs_open(default_loop, './test/support/fs/foo.txt', 0, mode_num('0666'), null);

    var len = uv.fs_read(default_loop, fd, buf, 0, null);
    assert(len === 10, 'should have read 10 bytes'); // 'some text\n'

    var str = new StringView(buf, 'utf-8', 0, len);
    assert(str.toString() === 'some text\n');

    uv.fs_close(default_loop, fd, null);
});

test('fs_readdir', function(done) {
    done = after(2, done);

    test_fs_fn(uv.fs_readdir, default_loop, './test/support/fs', 0, function(err, dirs) {
        assert.ifError(err);

        assert(dirs.length === 2, '2 files in support dir');
        assert(dirs.shift() === 'foo.txt', 'first file is foo.txt');
        assert(dirs.pop() === 'readme', 'last file is readme');
        done();
    });
});

