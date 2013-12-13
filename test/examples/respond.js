var uv = require('uv');

var assert = require('../support/assert');
var TextEncoder = require('./encoding').TextEncoder;

var tcp_handle = uv.tcp_init(uv.default_loop());

tcp_handle.bind({
    port: 9012,
    family: 'IPv4',
    address: '0.0.0.0'
});

var encoder = new TextEncoder('utf-8');

var headers = 'HTTP/1.1 200 OK\r\n' +
              'Server: TCPTest\r\n' +
              'Content-Type: text/plain; charset=latin-1\r\n' +
              'Connection: keep-alive\r\n' +
              'Content-Length: 12\r\n\r\n' +
              'hello world!';
var buf = encoder.encode(headers).buffer;

var handle_client = function(client) {
    var err = client.read_start(function(err, data) {
        if (err) {
            print(err);
            return client.close(function() {
                print('client closed');
            });
        }

        // eof
        if (!data) {
            client.close(function() {});
            return;
        }

        client.write(buf, function() {
        });
        client.close(function() {});
    });
};

var err = tcp_handle.listen(511, function(status) {
    var client = tcp_handle.accept();
    handle_client(client);
});

uv.run(uv.default_loop(), uv.UV_RUN_DEFAULT);
