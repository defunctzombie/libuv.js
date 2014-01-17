var test = require('./support/test');
var uv = require('uv');

var assert = require('./support/assert');
var StringView = require('./support/StringView');
var TextEncoder = require('./support/encoding').TextEncoder;

var encoder = new TextEncoder('utf-8');

var tcp_handle;

test('init', function() {
    tcp_handle = uv.tcp_init(uv.default_loop());
});

test('bind', function() {
    tcp_handle.bind({
        port: 8080,
        family: 'IPv4',
        address: '0.0.0.0'
    });
});

test('listen', function(done) {
    var err = tcp_handle.listen(0, function(server, status) {
        // will accept the connection
        // and makes a new handle for it
        var client = tcp_handle.accept();

        var err = client.read_start(function(err, data) {
            if (err) {
                return done(err);
            }

            // eof
            if (!data) {
                tcp_handle.close(function() {});
                return;
            }

            var str = new StringView(data);
            assert(str == 'ping');

            var buf = encoder.encode('pong').buffer;
            client.write(buf, function() {
            });
        });

        assert(err == 0); // success starting read
    });

    assert(err == 0);
    done();
});

test('client', function(done) {
    var ping_handle = uv.tcp_init(uv.default_loop());
    ping_handle.connect({ address: '127.0.0.1', port: 8080, family: 'IPv4'}, function() {
        ping_handle.read_start(function(err, data) {
            if (err) {
                print(err);
            }

            if (!data) {
                assert(false);
            }

            var str = new StringView(data);
            assert(str == 'pong');
            ping_handle.close(function() {
                done();
            });
        });

        var buf = encoder.encode('ping').buffer;
        var err = ping_handle.write(buf, function() {
        });
    });
});
