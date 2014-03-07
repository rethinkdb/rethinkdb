/*
  Copyright (C) 2013 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2012 Ariya Hidayat <ariya.hidayat@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*global require:true, console:true, process:true */

var fs = require('fs'),
    path = require('path'),
    esrefactor = require('../lib/esrefactor'),
    args = process.argv.splice(2),
    total = 0,
    failures = 0;

function testError(name, fragment) {
    var i, line, lines, matches, expected_error, func, context;

    console.log('Checking', name, 'for a thrown exception...');

    lines = fragment.split('\n');
    for (i = 0; i < lines.length; ++i) {
        line = lines[i];
        matches = /\/\/\s*exception:(.+)/.exec(line);
        if (matches) {
            expected_error = matches[1].trim();
        }
    }

    ++total;

    if (!expected_error) {
        ++failures;
        console.log('  FAIL: Can not find the expected error message');
        return;
    }

    fragment = '(function(context) {\n' +
      'try {\n' +
      fragment +
      '\n} catch(e) { context.lastError = e.message }\n' +
      '}) (arguments[0])';
    func = new Function(fragment);

    context = new esrefactor.Context();
    context.lastError = null;
    func.call(this, context);
    if (!context.lastError) {
        console.log('  FAIL: Expection is not thrown');
        console.log('    Expected:', expected_error);
        ++failures;
        return;
    } else if (context.lastError !== expected_error) {
        console.log('  FAIL: Mismatched thrown exception');
        console.log('    Expected:', expected_error);
        console.log('      Actual:', context.lastError);
        console.log();
        ++failures;
        return;
    }
}

function findMarker(line, marker) {
    var result;
    if (/\/\/.+/.exec(line)) {
        line.substr(line.indexOf('//')).split(' ').forEach(function (segment) {
            var id, pos;
            if (segment.substr(0, marker.length) === marker) {
                if (segment[marker.length] === ':') {
                    id = segment.substr(marker.length + 1).trim();
                    pos = 0;
                    while (pos >= 0) {
                        pos = line.indexOf(id, pos);
                        if (pos >= line.indexOf('//')) {
                            break;
                        }
                        if (!/[a-zA-Z0-9_]/.exec(line[pos + id.length])) {
                            result = {
                                name: id,
                                index: pos
                            };
                        }
                        ++pos;
                    }
                }
            }
        });
    }
    return result;
}

function sortReferences(references) {
    references.sort(function (ref) {
        return ref.range[0] - ref.range[1];
    });
}

function test(name, fixture) {
    var i, line, lines, offset, marker, range, cursor, declaration,
        references, context, result;

    console.log('Checking', name, '...');

    offset = 0;
    lines = fixture.split('\n');
    references = [];
    for (i = 0; i < lines.length; ++i) {
        line = lines[i];
        marker = findMarker(line, 'cursor');
        if (marker) {
            cursor = { type: 'Identifier', name: marker.name, range: []};
            cursor.range[0] = offset + marker.index;
            cursor.range[1] = offset + marker.index + marker.name.length;
        }
        marker = findMarker(line, 'declaration');
        if (marker) {
            declaration = { type: 'Identifier', name: marker.name, range: []};
            declaration.range[0] = offset + marker.index;
            declaration.range[1] = offset + marker.index + marker.name.length;
        }
        marker = findMarker(line, 'reference');
        if (marker) {
            references.push({
                type: 'Identifier',
                name: marker.name,
                range: [
                    offset + marker.index,
                    offset + marker.index + marker.name.length
                ]
            });
        }
        offset += (line.length + 1);
    }

    ++total;

    if (!cursor) {
        ++failures;
        console.log('  FAIL: Can not place a cursor');
        return;
    }

    if (!declaration) {
        declaration = null;
    }

    context = new esrefactor.Context(fixture);
    result = context.identify(cursor.range[0]);

    if (!result) {
        console.log('  FAIL: Identifier can not be located');
        ++failures;
        return;
    }

    if (JSON.stringify(result.identifier) !== JSON.stringify(cursor)) {
        console.log('  FAIL: Mismatched cursor');
        console.log('    Expected:', JSON.stringify(cursor));
        console.log('      Actual:', JSON.stringify(result.identifier));
        console.log();
        ++failures;
        return;
    }

    if (JSON.stringify(result.declaration) !== JSON.stringify(declaration)) {
        console.log('  FAIL: Mismatched declaration');
        console.log('    Expected:', JSON.stringify(declaration));
        console.log('      Actual:', JSON.stringify(result.declaration));
        console.log();
        ++failures;
    }

    // Sort the references based on location
    sortReferences(references);
    sortReferences(result.references);

    if (JSON.stringify(result.references) !== JSON.stringify(references)) {
        console.log('  FAIL: Mismatched references');
        console.log('    Expected:', JSON.stringify(references));
        console.log('      Actual:', JSON.stringify(result.references));
        console.log();
        ++failures;
    }

    // Check renaming the identifier.
    ['$', '$$', '_', '$unique', 'loooooong_name'].forEach(function (name ) {
        var renamed;

        function replace(line, marker) {
            if (marker && line.substr(marker.index, marker.name.length) === marker.name) {
                line = line.substr(0, marker.index) + name +
                    line.substr(marker.index + marker.name.length);
            }
            return line;
        }

        renamed = [];
        lines.forEach(function (line) {
            line = replace(line, findMarker(line, 'reference'));
            line = replace(line, findMarker(line, 'declaration'));
            renamed.push(line);
        });
        renamed = renamed.join('\n');

        result.renamed = context.rename(context.identify(cursor.range[0]), name);
        if (!result.renamed) {
            console.log('  FAIL: Unsuccesful renaming');
            ++failures;
            return;
        }

        if (renamed !== result.renamed) {
            console.log('  FAIL: Mismatched renaming');
            console.log('    Expected:', JSON.stringify(renamed.split('\n')));
            console.log('      Actual:', JSON.stringify(result.renamed.split('\n')));
            console.log();
            ++failures;
        }

        // Check that invalid identification should not change the code.
        result.renamed = context.rename(undefined, name);
        if (fixture !== result.renamed) {
            console.log('  FAIL: Wrong handling of invalid identification');
            console.log('    Expected:', JSON.stringify(fixutre.split('\n')));
            console.log('      Actual:', JSON.stringify(result.renamed.split('\n')));
            console.log();
            ++failures;
        }
    });

    // Check that passing a syntax tree also works.
    context.setCode(context._syntax);

    // Check that moving back the cursor doesn't hit any identifier.
    result = context.identify(cursor.range[0] - 1);
    if (result) {
        console.log('  FAIL: Unexpected identifier near the cursor');
        console.log('     ', JSON.stringify(result.identifier));
        console.log('     ', JSON.stringify(result.declaration));
        console.log();
        ++failures;
    }
}

// http://stackoverflow.com/q/5827612/
function walk(dir, done) {
    var results = [];
    fs.readdir(dir, function (err, list) {
        if (err) {
            return done(err);
        }
        var i = 0;
        (function next() {
            var file = list[i++];
            if (!file) {
                return done(null, results);
            }
            file = dir + '/' + file;
            fs.stat(file, function (err, stat) {
                if (stat && stat.isDirectory()) {
                    walk(file, function (err, res) {
                        results = results.concat(res);
                        next();
                    });
                } else {
                    results.push(file);
                    next();
                }
            });
        }());
    });
}


function processFile(filename) {
    var name, content;

    name = path.basename(filename);
    content = fs.readFileSync(filename, 'utf-8');
    if (name.match(/^error_/)) {
        testError(name, content);
    } else {
        test(name, content);
    }
}

function processDir(dirname) {

    walk(path.resolve(__dirname, dirname), function (err, results) {
        if (err) {
            console.log('Error', err);
            return;
        }

        results.forEach(processFile);

        console.log();
        console.log('Tests:', total, '  Failures:', failures);
        process.exit(failures === 0 ? 0 : 1);
    });
}

if (args.length === 0) {
    processDir('data');
} else {
    args.forEach(processFile);
}

