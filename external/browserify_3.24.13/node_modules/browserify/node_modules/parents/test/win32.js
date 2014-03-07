var test = require('tap').test;
var parents = require('../');

test('win32', function (t) {
    var dir = 'C:\\Program Files\\Maxis\\Sim City 2000\\cities';
    var dirs = parents(dir, { platform : 'win32' });
    t.same(dirs, [
        'C:\\Program Files\\Maxis\\Sim City 2000\\cities',
        'C:\\Program Files\\Maxis\\Sim City 2000',
        'C:\\Program Files\\Maxis',
        'C:\\Program Files',
        'C:',
    ]);
    t.end();
});

test('win32 c:', function (t) {
    var dirs = parents('C:\\', { platform : 'win32' });
    t.same(dirs, [ 'C:' ]);
    t.end();
});

test('win32 network drive', function (t) {
    var dirs = parents(
        '\\storageserver01\\Active Projects\\ProjectA',
        { platform : 'win32' }
    );
    t.same(dirs, [
        '\\storageserver01\\Active Projects\\ProjectA',
        '\\storageserver01\\Active Projects',
        '\\storageserver01'
    ]);
    t.end();
});
