var test = require('../');

var childRan = false;

test('parent', function(t) {
    t.test('child', function(t) {
        childRan = true;
        t.pass('child ran');
        t.end();
    });
    t.end();
});

test('uncle', function(t) {
    t.ok(childRan, 'Child should run before next top-level test');
    t.end();
});

var grandParentRan = false;
var parentRan = false;
var grandChildRan = false;
test('grandparent', function(t) {
    grandParentRan = true;
    t.test('parent', function(t) {
        parentRan = true;
        t.test('grandchild', function(t) {
            grandChildRan = true;
            t.pass('grand child ran');
            t.end();
        });
        t.pass('parent ran');
        t.end();
    });
    t.test('other parent', function(t) {
        t.ok(parentRan, 'first parent runs before second parent');
        t.ok(grandChildRan, 'grandchild runs before second parent');
        t.end();
    });
    t.pass('grandparent ran');
    t.end();
});

test('second grandparent', function(t) {
    t.ok(grandParentRan, 'grandparent ran');
    t.ok(parentRan, 'parent ran');
    t.ok(grandChildRan, 'grandchild ran');
    t.pass('other grandparent ran');
    t.end();
});

// vim: set softtabstop=4 shiftwidth=4:
