var tests = [];
var testing = false;

var c_red='\x1B[31m';
var c_green='\x1B[32m';
var c_default='\x1B[0m';

var next_test = function() {
    if (testing) {
        return;
    }

    var next = tests.shift();
    if (!next) {
        return;
    }

    testing = true;

    var name = next.name;
    var test_fn = next.cb;

    var done = function(err) {
        if (err) {
            print(c_red + '×' + c_default + ' ' + name)
            print(err.stack);
        }
        else {
            print(c_green + '✓' + c_default + ' ' + name)
        }

        testing = false;
        next_test();
    };

    if (test_fn.length === 1) {
        return test_fn(done);
    }

    try {
        test_fn();
        done();
    } catch (err) {
        done(err);
    }
};

return function(name, cb) {
    tests.push({
        name: name,
        cb: cb
    });

    next_test();
}
