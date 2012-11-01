// Copyright 2010-2012 RethinkDB, all rights reserved.
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

    r.branch(r.expr(false), r.expr(1), r.error()).runp();
    r.table('test').orderby('bob').runp();
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

    r.expr([1,2,r.error()]).arrayToStream().runp();

    r.db('test').create('test').runp();
    r.db('bob').create({
        tableName: 'test',
        cacheSize: '1234'
    }).runp();

    r.db('test').drop('alphabet').runp();
    r.db('test').table('lakne').runp();
    r.db('test').table('test').get(r.error()).runp();
    r.db('test').table('test').insert({id:r.error()}).runp();
    r.db('test').table('bob').del().runp();
    r.table('test').get(r.error()).del().runp();

    r.table('bo').update(function(a) {return r.error()}).runp();
    r.table('test').get(1).update(function(row) {return r.error()}).runp();
    r.table('bo').replace(function(a) {return r.error()}).runp();
    r.table('test').get(1).replace(function(row) {return r.error()}).runp();

    r.table('test').forEach(function(a) {
        return r.table('bo').insert(r.error());
    }).runp();

    r.table('test').between(0, 1, 'bob').runp();

    r.table('test').insert(r.expr([{id:{}}]).arrayToStream()).runp();

    r.table('test').get(0).update(function(val) {return val.extend({'bob':1})}).runp();

    r.table('test').get(1, 'bob').update(function(row) {return row.extend({bob:3})}).runp();
    r.table('test').get(1, 'bob').replace(function(row) {return row.extend({bob:3})}).runp();
    r.table('test').get(1, 'bob').del().runp();
    r.table('test').insert({id:1}).runp();

    //c.close();
});
