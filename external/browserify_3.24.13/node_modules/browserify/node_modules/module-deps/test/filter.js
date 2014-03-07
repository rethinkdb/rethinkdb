var test = require('tape');
var path  = require('path')
var mdeps = require('../')

var core = ['events', 'util', 'dns', 'dgram', 'http', 'https', 'net', 'fs']

var collect = []

var entry = path.join(__dirname, 'files', 'filterable.js')

test('can filter core deps', function (t) {

    mdeps(entry, {
        filter: function (e) {
            return !~core.indexOf(e)
        }
    })
    .on('data', function (d) {
        collect.push(d)
        t.equal(d.id, entry)
        t.deepEqual(d.deps, {
            events: false,
            fs: false,
            net: false,
            http: false,
            https: false,
            dgram: false,
            dns: false
        })
        t.equal(d.entry, true)
    })
    .on('end', function () {
        t.equal(collect.length, 1)
        t.end()
    })

})
