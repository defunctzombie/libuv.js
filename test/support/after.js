return function(count, fn) {
    return function() {
        if (--count <= 0) {
            fn();
        }
    }
};
