var r = require('./rethinkdb');

var one = r.expr(1);
var two = r.expr(2);
var zero = r.expr(0);

var arr = r.expr([1,2,3]);

var c = r.connect({}, function() {
    c.setErrorHandler(function(err) {
        console.log(err.name+': '+err.message);
    });

    r.expr('a').add(1).runp();
    arr.map(function(row) { return row('a'); }).runp();
    arr.reduce('a', function(acc, row) {return row.add(acc)}).runp();
    r.expr([{a:1}]).filter({a:one.div(zero)}).runp();

    c.close();
});
