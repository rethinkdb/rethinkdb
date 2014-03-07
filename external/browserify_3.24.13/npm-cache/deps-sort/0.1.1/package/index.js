var through = require('through');

module.exports = function (opts) {
    if (!opts) opts = {};
    
    var rows = [];
    return through(write, end);
    
    function write (row) { rows.push(row) }
    
    function end () {
        var tr = this;
        rows.sort(cmp);
        
        if (opts.index) {
            var index = {};
            rows.forEach(function (row, ix) {
                row.index = ix + 1;
                index[row.id] = ix + 1;
            });
            rows.forEach(function (row) {
                row.indexDeps = {};
                Object.keys(row.deps).forEach(function (key) {
                    row.indexDeps[key] = index[row.deps[key]];
                });
                tr.queue(row);
            });
        }
        else {
            rows.forEach(function (row) {
                tr.queue(row);
            });
        }
        tr.queue(null);
    }
    
    function cmp (a, b) {
        return a.id < b.id ? -1 : 1;
    }
};
