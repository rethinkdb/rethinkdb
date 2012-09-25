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

    r.ifThenElse(r.expr(false), r.expr(1), r.error()).runp();
    r.table('test').orderby('id').runp();
    r.expr([{id:0},{id:1}]).distinct().runp();

    r.expr([1,2,3]).groupedMapReduce(function(row) {
        return row;
    }, function(row) {
        return row;
    }, r.error(), function(acc, row) {
        return r.error();
    }).runp();

    r.expr({id:r.error()}).contains('b').runp();
    r.expr({id:1}).pick('a').runp();
    r.expr({a:r.error()}).extend({b:1}).runp();
    r.expr({a:1}).extend({b:r.error()}).runp();
    r.expr([1,2]).concatMap(function(a) {return r.expr([r.error()])}).runp();

    c.close();
});
