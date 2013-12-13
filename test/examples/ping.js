var uv = require('uv');

var assert = require('../support/assert');
var StringView = require('../support/StringView');

var tcp_handle = uv.tcp_init(uv.default_loop());

tcp_handle.bind({
    port: 8080,
    family: 'IPv4',
    address: '0.0.0.0'
});

var err = tcp_handle.listen(0, function(server, status) {
    //print('>connection');

    // will accept the connection
    // and makes a new handle for it
    var client = server.accept();

    //print(">accepted");

    var err = client.read_start(function(err, data) {
        if (err) {
            print(err);
            return client.close(function() {
                print('client closed');
            });
        }

        // eof
        if (!data) {
            tcp_handle.close(function() {});
            return;
        }

        //var str = new StringView(data);
        //print('got:', str);

        client.write([data], function() {
            print('server wrote back');
        });
    });
    assert(err == 0); // success starting read
});

var count = 0;
var ping_handle = uv.tcp_init(uv.default_loop());
ping_handle.connect({ address: '127.0.0.1', port: 8080, family: 'IPv4'}, function() {
    print('connected client');

    ping_handle.read_start(function(err, data) {
        if (err) {
            print(err);
        }

        if (!data) {
        }

        //var str = new StringView(data);
        //print('client got:', str);
        count++;
        //assert(str == 'hello world!');

        var buf = strToUTF8Arr('hello world!').buffer;
        var err = ping_handle.write([buf], function() {
            print('written');
        });

    });

    var buf = strToUTF8Arr('hello world!').buffer;
    var err = ping_handle.write([buf], function() {
        print('written');
    });

    var timer = uv.timer_init(uv.default_loop());
    timer.start(1000, 0, function() {
        print('ping count', count);
        ping_handle.close(function() {
        });
    });
});

uv.run(uv.default_loop(), uv.UV_RUN_DEFAULT);

function strToUTF8Arr (sDOMStr) {

    var aBytes, nChr, nStrLen = sDOMStr.length, nArrLen = 0;

    /* mapping... */

    for (var nMapIdx = 0; nMapIdx < nStrLen; nMapIdx++) {
        nChr = sDOMStr.charCodeAt(nMapIdx);
        nArrLen += nChr < 0x80 ? 1 : nChr < 0x800 ? 2 : nChr < 0x10000 ? 3 : nChr < 0x200000 ? 4 : nChr < 0x4000000 ? 5 : 6;
    }

    aBytes = new Uint8Array(nArrLen);

    /* transcription... */

    for (var nIdx = 0, nChrIdx = 0; nIdx < nArrLen; nChrIdx++) {
        nChr = sDOMStr.charCodeAt(nChrIdx);
        if (nChr < 128) {
            /* one byte */
            aBytes[nIdx++] = nChr;
        } else if (nChr < 0x800) {
            /* two bytes */
            aBytes[nIdx++] = 192 + (nChr >>> 6);
            aBytes[nIdx++] = 128 + (nChr & 63);
        } else if (nChr < 0x10000) {
            /* three bytes */
            aBytes[nIdx++] = 224 + (nChr >>> 12);
            aBytes[nIdx++] = 128 + (nChr >>> 6 & 63);
            aBytes[nIdx++] = 128 + (nChr & 63);
        } else if (nChr < 0x200000) {
            /* four bytes */
            aBytes[nIdx++] = 240 + (nChr >>> 18);
            aBytes[nIdx++] = 128 + (nChr >>> 12 & 63);
            aBytes[nIdx++] = 128 + (nChr >>> 6 & 63);
            aBytes[nIdx++] = 128 + (nChr & 63);
        } else if (nChr < 0x4000000) {
            /* five bytes */
            aBytes[nIdx++] = 248 + (nChr >>> 24);
            aBytes[nIdx++] = 128 + (nChr >>> 18 & 63);
            aBytes[nIdx++] = 128 + (nChr >>> 12 & 63);
            aBytes[nIdx++] = 128 + (nChr >>> 6 & 63);
            aBytes[nIdx++] = 128 + (nChr & 63);
        } else /* if (nChr <= 0x7fffffff) */ {
            /* six bytes */
            aBytes[nIdx++] = 252 + /* (nChr >>> 32) is not possible in ECMAScript! So...: */ (nChr / 1073741824);
            aBytes[nIdx++] = 128 + (nChr >>> 24 & 63);
            aBytes[nIdx++] = 128 + (nChr >>> 18 & 63);
            aBytes[nIdx++] = 128 + (nChr >>> 12 & 63);
            aBytes[nIdx++] = 128 + (nChr >>> 6 & 63);
            aBytes[nIdx++] = 128 + (nChr & 63);
        }
    }

    return aBytes;

}
