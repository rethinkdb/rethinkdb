var browserify = require('../');
var path = require('path');
var spawn = require('child_process').spawn;
var parseShell = require('shell-quote').parse;
var duplexer = require('duplexer');
var minimist = require('minimist');

module.exports = function (args) {
    var argv = minimist(args, {
        'boolean': [
            'deps','pack','ig','dg', 'im', 'd','list',
            'builtins','commondir', 'bare'
        ],
        string: [ 's' ],
        alias: {
            ig: [ 'insert-globals', 'fast' ],
            dg: 'detect-globals',
            im: 'ignore-missing',
            igv: 'insert-global-vars',
            d: 'debug',
            s: 'standalone',
            noparse: 'noParse',
            bare: 'bear'
        },
        'default': {
            ig: false,
            im: false,
            dg: true,
            d: false,
            builtins: true,
            commondir: true
        }
    });
    
    var entries = argv._.concat(argv.e).concat(argv.entry)
    .filter(Boolean).map(function(entry) {
        if (entry === '-') return process.stdin;
        return path.resolve(process.cwd(), entry);
    });
    var requires = [].concat(argv.r, argv.require).filter(Boolean);
    
    if (entries.length + requires.length === 0 && !process.stdin.isTTY) {
        entries.push(process.stdin);
    }
    
    if (argv.s && entries.length === 0 && requires.length === 1) {
        entries.push(requires[0]);
        argv.r = argv.require = [];
    }
    
    if (argv.bare) {
        argv.builtins = false;
        argv.commondir = false;
        if (argv.igv === undefined) {
            argv.igv = '__filename,__dirname';
        }
    }
    
    var b = browserify({
        noParse: [].concat(argv.noparse).filter(Boolean),
        extensions: [].concat(argv.extension).filter(Boolean),
        entries: entries,
        builtins: argv.builtins === false ? false : undefined,
        commondir: argv.commondir === false ? false : undefined
    });
    b.argv = argv;
    
    [].concat(argv.i).concat(argv.ignore).filter(Boolean)
        .forEach(function (i) { b.ignore(i) })
    ;
    
    [].concat(argv.u).concat(argv.exclude).filter(Boolean)
        .forEach(function (u) { b.exclude(u) })
    ;
    
    [].concat(argv.r).concat(argv.require).filter(Boolean)
        .forEach(function (r) {
            var xs = r.split(':');
            b.require(xs[0], { expose: xs.length === 1 ? xs[0] : xs[1] })
        })
    ;
    
    // resolve any external files and add them to the bundle as externals
    [].concat(argv.x).concat(argv.external).filter(Boolean)
        .forEach(function (x) {
            var xs = x.split(':');
            if (xs.length === 1) {
                opts = {};
            } else {
                x = xs[0];
                opts = {expose: xs[1]};
            }
            if (/^[\/.]/.test(x)) b.external(path.resolve(process.cwd(), x),opts)
            else b.external(x,opts)
        })
    ;
    
    [].concat(argv.t).concat(argv.transform)
        .filter(Boolean)
        .forEach(function (t) { b.transform(t) })
    ;
    
    [].concat(argv.g).concat(argv['global-transform'])
        .filter(Boolean)
        .forEach(function (t) { b.transform({ global: true }, t) })
    ;
    
    [].concat(argv.c).concat(argv.command).filter(Boolean)
        .forEach(function (c) {
            var cmd = parseShell(c);
            b.transform(function (file) {
                var env = Object.keys(process.env).reduce(function (acc, key) {
                    acc[key] = process.env[key];
                    return acc;
                }, {});
                env.FILENAME = file;
                var ps = spawn(cmd[0], cmd.slice(1), { env: env });
                var error = '';
                ps.stderr.on('data', function (buf) { error += buf });
                
                ps.on('exit', function (code) {
                    if (code === 0) return;
                    console.error([
                        'error running source transform command: ' + c,
                        error.split('\n').join('\n  '),
                        ''
                    ].join('\n'));
                    process.exit(1);
                });
                return duplexer(ps.stdin, ps.stdout);
            });
        })
    ;
    
    if (argv.standalone === true) {
        process.nextTick(function () {
            b.emit('error', '--standalone requires an export name argument');
        });
        return b;
    }
    
    var bundleOpts = {
        detectGlobals: argv['detect-globals'] !== false && argv.dg !== false,
        insertGlobals: argv['insert-globals'] || argv.ig,
        insertGlobalVars: argv.igv ? argv.igv.split(',') : undefined,
        ignoreMissing: argv['ignore-missing'] || argv.im,
        debug: argv['debug'] || argv.d,
        standalone: argv['standalone'] || argv.s
    };
    var bundle = b.bundle;
    b.bundle = function (opts, cb) {
        if (!opts) opts = {};
        if (typeof opts === 'function') { cb = opts; opts = {} };
        var bopts = copy(bundleOpts);
        Object.keys(opts).forEach(function (key) {
            bopts[key] = opts[key];
        });
        return bundle.call(b, bopts, cb);
    };
    
    return b;
};

function copy (obj) {
    return Object.keys(obj).reduce(function (acc, key) {
        acc[key] = obj[key];
        return acc;
    }, {});
}
