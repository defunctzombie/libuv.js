var test = require('./support/test');
var assert = require('./support/assert');
var after = require('./support/after');
var uv = require('./support/uv');

var default_loop = uv.default_loop();

test('timeout', function(done) {
    var start = Date.now();

    var timer = uv.timer_init(default_loop);

    timer.start(500, 0, function() {
        var now = Date.now();
        var delta = Math.abs(start - now);
        assert(delta > 490 && delta < 510);
        done();
    });
});

test('gc', function() {
    gc();
});

test('stop timer', function(done) {

    var timer = uv.timer_init(default_loop);

    timer.start(200, 0, function() {
        assert(false);
    });

    uv.timer_init(default_loop).start(500, 0, done);

    gc();

    var res = timer.stop();
    assert(res === 0);

    gc();

    // double stop should work
    var res = timer.stop();
    assert(res === 0);

    gc();
});

test('gc', function() {
    gc();
});

test('repeat', function(done) {
    var start = Date.now();

    var timer = uv.timer_init(default_loop);

    var timeout = after(5, function() {
        timer.stop();
        timer = undefined;

        gc();
        done();
    });

    // this exposes a nasty bug
    // because handle is cleaned up
    timer.start(500, 100, function() {
        gc();
        timeout();
    });
});
