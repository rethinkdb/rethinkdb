#!/usr/bin/env node
var mdeps = require('./');
var argv = require('minimist')(process.argv.slice(2));
var JSONStream = require('JSONStream');

var stringify = JSONStream.stringify();
stringify.pipe(process.stdout);

var path = require('path');
var files = argv._.map(function (file) {
    return path.resolve(file);
});
mdeps(files, argv).pipe(stringify);
