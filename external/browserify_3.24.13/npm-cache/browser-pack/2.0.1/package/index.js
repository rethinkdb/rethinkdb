var JSONStream = require('JSONStream');
var through = require('through');

var fs = require('fs');
var path = require('path');

var combineSourceMap = require('combine-source-map');

var defaultPreludePath = path.join(__dirname, '_prelude.js');
var defaultPrelude = fs.readFileSync(defaultPreludePath, 'utf8');

function newlinesIn(src) {
  if (!src) return 0;
  var newlines = src.match(/\n/g);

  return newlines ? newlines.length : 0;
}

module.exports = function (opts) {
    if (!opts) opts = {};
    var parser = opts.raw ? through() : JSONStream.parse([ true ]);
    var stream = through(
        function (buf) { parser.write(buf) },
        function () { parser.end() }
    );
    parser.pipe(through(write, end));
    
    var first = true;
    var entries = [];
    var prelude = opts.prelude || defaultPrelude;
    var preludePath = opts.preludePath || defaultPreludePath;
    
    var lineno = 1 + newlinesIn(prelude);
    var sourcemap;
    
    return stream;
    
    function write (row) {
        if (first) stream.queue(prelude + '({');
        
        if (row.sourceFile && !row.nomap) {
            if (!sourcemap) {
                sourcemap = combineSourceMap.create();
                sourcemap.addFile(
                    { sourceFile: preludePath, source: prelude },
                    { line: 0 }
                );
            }
            sourcemap.addFile(
                { sourceFile: row.sourceFile, source: row.source },
                { line: lineno }
            );
        }
        
        var wrappedSource = [
            (first ? '' : ','),
            JSON.stringify(row.id),
            ':[',
            'function(require,module,exports){\n',
            combineSourceMap.removeComments(row.source),
            '\n},',
            '{' + Object.keys(row.deps || {}).sort().map(function (key) {
                return JSON.stringify(key) + ':'
                    + JSON.stringify(row.deps[key])
                ;
            }).join(',') + '}',
            ']'
        ].join('');

        stream.queue(wrappedSource);
        lineno += newlinesIn(wrappedSource);
        
        first = false;
        if (row.entry && row.order !== undefined) {
            entries[row.order] = row.id;
        }
        else if (row.entry) entries.push(row.id);
    }
    
    function end () {
        if (first) stream.queue(prelude + '({');
        entries = entries.filter(function (x) { return x !== undefined });
        
        stream.queue('},{},' + JSON.stringify(entries) + ')');
        if (sourcemap) {
            var comment = sourcemap.comment();
            if (opts.sourceMapPrefix) {
                comment = comment.replace(
                    /^\/\/#/, function () { return opts.sourceMapPrefix }
                )
            }
            stream.queue('\n' + comment);
        }

        stream.queue(null);
    }
};
