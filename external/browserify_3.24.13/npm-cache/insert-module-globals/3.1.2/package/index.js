var parseScope = require('lexical-scope');
var through = require('through');

var path = require('path');
var fs = require('fs');
var processModulePath = require.resolve('process/browser.js');

var defaultVars = {
    process: function () {
        return 'require(' + JSON.stringify(processModulePath) + ')';
    },
    global: function () {
        return 'typeof self !== "undefined" ? self : '
            + 'typeof window !== "undefined" ? window : {}'
        ;
    },
    Buffer: function () {
        return 'require("buffer").Buffer';
    },
    __filename: function (file, basedir) {
        var file = '/' + path.relative(basedir, file);
        return JSON.stringify(file);
    },
    __dirname: function (file, basedir) {
        var dir = path.dirname('/' + path.relative(basedir, file));
        return JSON.stringify(dir);
    }
};

module.exports = function (file, opts) {
    if (/\.json$/i.test(file)) return through();
    if (!opts) opts = {};
    
    var basedir = opts.basedir || '/';
    var vars = opts.vars || defaultVars
    var varNames = Object.keys(vars);
    
    var quick = RegExp(varNames.map(function (name) {
        return '\\b' + name + '\\b';
    }).join('|'));
    
    var resolved = {};
    var chunks = [];
    
    return through(write, end);
    
    function write (buf) { chunks.push(buf) }
    
    function end () {
        var source = Buffer.isBuffer(chunks[0])
            ? Buffer.concat(chunks).toString('utf8')
            : chunks.join('')
        ;
        source = source.replace(/^#![^\n]*\n/, '\n');
        
        if (opts.always !== true && !quick.test(source)) {
            this.queue(source);
            this.queue(null);
            return;
        }
        
        try {
            var scope = opts.always
                ? { globals: { implicit: varNames } }
                : parseScope('(function(){' + source + '})()')
            ;
        }
        catch (err) {
            var e = new SyntaxError(
                (err.message || err) + ' while parsing ' + file
            );
            e.type = 'syntax';
            e.filename = file;
            return this.emit('error', e);
        }
        
        var globals = {};
        
        varNames.forEach(function (name) {
            if (scope.globals.implicit.indexOf(name) >= 0) {
                var value = vars[name](file, basedir);
                if (value) globals[name] = value;
            }
        });
        
        this.queue(closeOver(globals, source));
        this.queue(null);
    }
};

module.exports.vars = defaultVars;

function closeOver (globals, src) {
    var keys = Object.keys(globals);
    if (keys.length === 0) return src;
    var values = keys.map(function (key) { return globals[key] });
    
    if (keys.length <= 3) {
        return '(function (' + keys.join(',') + '){'
            + src + '}).call(this,' + values.join(',') + ')'
        ;
    }
    // necessary to make arguments[3..6] still work for workerify etc
    // a,b,c,arguments[3..6],d,e,f...
    var extra = [ '__argument0', '__argument1', '__argument2', '__argument3' ];
    var names = keys.slice(0,3).concat(extra).concat(keys.slice(3));
    values.splice(3, 0,
        'arguments[3]','arguments[4]',
        'arguments[5]','arguments[6]'
    );
    
    return '(function (' + names.join(',') + '){'
        + src + '}).call(this,' + values.join(',') + ')'
    ;
}
