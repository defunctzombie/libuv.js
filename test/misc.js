var assert = require('./support/assert');
var uv = require('./support/uv');

test('version', function() {
    assert.ok(uv.version_string());
    assert.ok(typeof uv.version_string() === 'string');
});

test('now', function() {
    var now = uv.now(uv.default_loop());
    assert(now > 0);
});

test('enums', function() {
    assert(uv.UV_RUN_DEFAULT === 0);
    assert(uv.UV_RUN_ONCE === 1);
    assert(uv.UV_RUN_NOWAIT === 2);

    assert(uv.UV_LEAVE_GROUP === 0);
    assert(uv.UV_JOIN_GROUP === 1);
});
