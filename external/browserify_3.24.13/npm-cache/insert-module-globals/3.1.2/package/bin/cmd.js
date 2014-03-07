#!/usr/bin/env node

var insert = require('../');
var through = require('through');
var concat = require('concat-stream');
var JSONStream = require('JSONStream');

var basedir = process.argv[2] || process.cwd();

process.stdin
    .pipe(JSONStream.parse([ true ]))
    .pipe(through(write))
    .pipe(JSONStream.stringify())
    .pipe(process.stdout)
;

function write (row) {
    var self = this;
    var s = insert(row.id, { basedir: basedir });
    s.pipe(concat(function (src) {
        row.source = src.toString('utf8');
        self.queue(row);
    }));
    s.end(row.source);
}
