var exec = run;
delete this.run;

var loaded = {};

this.require = function(path) {
    path = path + '.js';
    var src = read(path);
    src = '(function(){\n' + src + '\n})()';
    var module = exec(src, path);

    loaded[path] = module;
    return module;
};
