var assert = function(bool, msg) {
    if (!bool) {
        throw new Error(msg);
    }
};

assert.ok = assert;

assert.ifError = function(err, msg) {
    if (err) {
        throw err;
    }
};

return assert;
