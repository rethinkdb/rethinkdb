var test = require('../');
var harness = test.createHarness({ exit : false });

// minimal write stream mockery
var collector = {
    written: [],
    ended: false,
    writable: true,
    write: function (c) {
        console.log('#> %s', c.replace(/\n$/, '').split(/\n/).join('\n#> '));
        if (this.ended)
            throw new Error('write after end');
        this.written.push(c);
        return true;
    },
    end: function (c) {
        console.error('collector end');
        if (this.ended)
            throw new Error('end after end');
        if (c)
            this.write(c);
        this.ended = true;
    },
    on: function () {},
    emit: function () {},
    removeListener: function () {}
};

var wanted =
    [ 'TAP version 13',
      '# harness test',
      '# hello',
      'ok 1 this is ok',
      'not ok 2 this is not ok',
      '  ---',
      '    operator: fail',
      '    expected:',
      '    actual:',
      '  ...',
      '# sub harness test',
      'ok 3 math still works',
      'ok 4 except when it doesn\'t',
      '# sub harness internal test',
      'ok 5 gas',
      'ok 6 lulz i am immature',
      '',
      '1..6',
      '# tests 6',
      '# pass  5',
      '# fail  1',
      '' ];

harness.stream.pipe(collector);

test('correct output', function (t) {
    harness.stream.on('end', function () {
        // accept trailing whitespace, or multiple lines on the same write(),
        var found = collector.written.join('').split(/\n/).map(function (s) {
            return s.replace(/\s+$/, '');
        });
        t.same(found, wanted, 'got expected output');
        t.end();
    });

    harness('harness test', function (ht) {
        // console.error('harness stream', harness.stream);
        ht.comment('hello');
        ht.pass('this is ok');
        ht.fail('this is not ok');
        ht.test('sub harness test', function (sht) {
            sht.equal(4, 4, 'math still works');
            sht.notEqual(0.1 + 0.1 + 0.1, 0.3, 'except when it doesn\'t');
            sht.test('sub harness internal test', function (shit) {
                console.error('sub harness internal test');
                // shit.plan(2);
                shit.pass('gas');
                shit.pass('lulz i am immature');
                shit.end();
            });
            sht.end();
        });
        ht.end();
        console.error('called ht.end');
    });
});


// vim: set softtabstop=4 shiftwidth=4:
