var test = require('./support/test');
var assert = require('./support/assert');
var after = require('./support/after');
var uv = require('./support/uv');

var default_loop = uv.default_loop();

test('stream', function(done) {
    var tcp_handle = uv.tcp_init(default_loop);

    var err = tcp_handle.bind({
        port: 8080,
        family: 'IPv4',
        address: '0.0.0.0'
    });
    assert(err == 0);

    var err = tcp_handle.listen(0, function(server, status) {
        assert(false);
    });

    var socket_info = tcp_handle.getsockname();
    assert(err === 0);

    tcp_handle.close(function(err) {
        done();
    });
});

