var tap = require('tap');
var tape = require('../');

tap.test('tape only test', function (tt) {
    var test = tape.createHarness({ exit: false });
    var tc = tap.createConsumer();

    var rows = []
    tc.on('data', function (r) { rows.push(r) })
    tc.on('end', function () {
        var rs = rows.map(function (r) {
            if (r && typeof r === 'object') {
                return { id: r.id, ok: r.ok, name: r.name.trim() };
            }
            else {
                return r;
            }
        })

        tt.deepEqual(rs, [
            'TAP version 13',
            'run success',
            { id: 1, ok: true, name: 'assert name'},
            'tests 1',
            'pass  1',
            'ok'
        ])

        tt.end()
    })

    test.stream.pipe(tc)

    test("never run fail", function (t) {
        t.equal(true, false)
        t.end()
    })

    test("never run success", function (t) {
        t.equal(true, true)
        t.end()
    })

    test.only("run success", function (t) {
        t.ok(true, "assert name")
        t.end()
    })
})
