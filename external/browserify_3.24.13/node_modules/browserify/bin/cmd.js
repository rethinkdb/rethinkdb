#!/usr/bin/env node
var fs = require('fs');
var JSONStream = require('JSONStream');
var through = require('through');

var b = require('./args')(process.argv.slice(2));
process.stdout.on('error', process.exit);

if ((b.argv._[0] === 'help' && b.argv._[1]) === 'advanced'
|| (b.argv.h || b.argv.help) === 'advanced') {
    return fs.createReadStream(__dirname + '/advanced.txt')
        .pipe(process.stdout)
        .on('close', function () { process.exit(1) })
    ;
}
if (b.argv._[0] === 'help' || b.argv.h || b.argv.help
|| (process.argv.length <= 2 && process.stdin.isTTY)) {
    return fs.createReadStream(__dirname + '/usage.txt')
        .pipe(process.stdout)
        .on('close', function () { process.exit(1) })
    ;
}
if (b.argv.v || b.argv.version) {
    return console.log(require('../package.json').version);
}

b.on('error', function (err) {
    console.error(String(err));
    process.exit(1);
});

if (b.argv.pack) {
    process.stdin.pipe(b.pack()).pipe(process.stdout);
    process.stdin.resume();
    return;
}

if (b.argv.deps) {
    var stringify = JSONStream.stringify();
    var t = [].concat(b.argv.t).concat(b.argv.transform);
    var d = b.deps({ packageFilter: packageFilter, transform: t });
    d.pipe(stringify).pipe(process.stdout);
    return;
}

if (b.argv.list) {
    var t = [].concat(b.argv.t).concat(b.argv.transform);
    var d = b.deps({ packageFilter: packageFilter, transform: t });
    d.pipe(through(function (dep) {
        this.queue(dep.id + '\n');
    })).pipe(process.stdout);
    return;
}

var bundle = b.bundle();
bundle.on('error', function (err) {
    console.error(String(err));
    process.exit(1);
});

var outfile = b.argv.o || b.argv.outfile;
if (outfile) {
    bundle.pipe(fs.createWriteStream(outfile));
}
else {
    bundle.pipe(process.stdout);
}

function packageFilter (info) {
    if (info && typeof info.browserify === 'string' && !info.browser) {
        info.browser = info.browserify;
        delete info.browserify;
    }
    return info || {};
}
