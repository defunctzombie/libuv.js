var test = require('./support/test');
var assert = require('./support/assert');
var uv = require('./support/uv');

require('./misc');
require('./timer');
require('./fs');
require('./stream');
require('./tcp');

// launch our loop, without this some tests won't run
var loop = uv.default_loop();
var result = uv.run(loop, uv.UV_RUN_DEFAULT);

test('done', function() {
    assert(result === 0);
});
