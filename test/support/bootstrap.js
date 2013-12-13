var read_file = read;
var argv = argv;
var cwd = cwd;
var uv_bindings = __uv_bindings;

var program = minimist(argv.slice(1));

var script = cwd + '/' + program._

var global = {};

var cache = {};

function new_require(path) {
    return function(name) {
        if (name === 'uv') {
            return uv_bindings;
        }

        var fullpath = path + '/' + name + '.js';
        if (cache[fullpath]) {
            return cache[fullpath];
        }

        var src = read_file(fullpath);
        var new_path = fullpath.split('/').slice(0, -1).join('/');

        var old_require = global.require;
        global.require = new_require(new_path);

        src = '(function(global, require){' + src + '}).call(null, global, global.require);';

        var res = run(src, fullpath);
        cache[fullpath] = res;

        // put back cause we return control back to parent
        global.require = old_require;

        return res;
    }
}

// get path of script file from fullpath
var script_path = script.split('/').slice(0, -1).join('/');

var ctx = {};
ctx.require = new_require(script_path);

var src = read_file(script);

// prime require with our current script path
global.require = new_require(script_path);

// will run using the object as the new context
src = '(function(global, require){' + src + '}).call(null, global, global.require);';
run(src, script, global);

// https://raw.github.com/substack/minimist/master/index.js
function minimist(args) {

    var argv = { _ : [] };

    var notFlags = [];

    if (args.indexOf('--') !== -1) {
        notFlags = args.slice(args.indexOf('--')+1);
        args = args.slice(0, args.indexOf('--'));
    }

    function setArg (key, val) {
        argv[key] = val;
    }

    for (var i = 0; i < args.length; i++) {
        var arg = args[i];

        if (arg.match(/^--.+=/)) {
            // Using [\s\S] instead of . because js doesn't support the
            // 'dotall' regex modifier. See:
            // http://stackoverflow.com/a/1068308/13216
            var m = arg.match(/^--([^=]+)=([\s\S]*)$/);
            setArg(m[1], m[2]);
        }
        else if (arg.match(/^--no-.+/)) {
            var key = arg.match(/^--no-(.+)/)[1];
            setArg(key, false);
        }
        else if (arg.match(/^--.+/)) {
            var key = arg.match(/^--(.+)/)[1];
            var next = args[i + 1];
            if (next !== undefined && !next.match(/^-/)) {
                setArg(key, next);
                i++;
            }
            else if (/^(true|false)$/.test(next)) {
                setArg(key, next === 'true');
                i++;
            }
            else {
                setArg(key, true);
            }
        }
        else {
            argv._.push(arg);
        }
    }

    notFlags.forEach(function(key) {
        argv._.push(key);
    });

    return argv;
};
