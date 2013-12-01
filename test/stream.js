var test = require('./support/test');
var assert = require('./support/assert');
var after = require('./support/after');
var uv = require('./support/uv');

var default_loop = uv.default_loop();

test('stream', function(done) {

    var stream = uv.stream_new();

    var res = uv.tcp_init(default_loop, stream);
    assert(res === 0);

    var res = uv.listen(stream, 0, function(server, status) {
        // new connection
        assert(false);
    });
    assert(res === 0);

    uv.close(stream, function(err) {
        done();
    });
});
