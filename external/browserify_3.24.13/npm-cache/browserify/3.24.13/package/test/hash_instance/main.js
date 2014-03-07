var Foo1 = require('./one.js');
var Foo2 = require('./foo/two.js');

var f1 = new Foo1;
var f2 = new Foo2;

t.equal(Foo1, Foo2);
t.ok(f1 instanceof Foo1);
t.ok(f1 instanceof Foo2);
t.ok(f2 instanceof Foo1);
t.ok(f2 instanceof Foo2);
