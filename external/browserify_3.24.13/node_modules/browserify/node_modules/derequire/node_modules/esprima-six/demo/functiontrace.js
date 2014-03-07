/*
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

/*jslint browser:true evil:true */

(function (global) {
    'use strict';

    var lookup;

    function id(i) {
        return document.getElementById(i);
    }

    function escaped(str) {
        return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    function setText(id, str) {
        var el = document.getElementById(id);
        if (typeof el.innerText === 'string') {
            el.innerText = str;
        } else {
            el.textContent = str;
        }
    }

    function isLineTerminator(ch) {
        return (ch === '\n' || ch === '\r' || ch === '\u2028' || ch === '\u2029');
    }

    // Remove all previous tracing.
    function untrace(code) {

        var i, trace, traces = [];

        // Executes visitor on the object and its children (recursively).
        function traverse(object, visitor, master) {
            var key, child, parent, path;

            parent = (typeof master === 'undefined') ? [] : master;

            if (visitor.call(null, object, parent) === false) {
                return;
            }
            for (key in object) {
                if (object.hasOwnProperty(key)) {
                    child = object[key];
                    path = [ object ];
                    path.push(parent);
                    if (typeof child === 'object' && child !== null) {
                        traverse(child, visitor, path);
                    }
                }
            }
        }

        // In order to remove all previous tracing, we need to find the syntax
        // nodes associated with the tracing.
        traverse(global.esprima.parse(code, { range: true }), function (node, parent) {
            var n = node;

            if (n.type !== 'CallExpression') {
                return;
            }
            n = n.callee;

            if (!n || n.type !== 'MemberExpression') {
                return;
            }
            if (n.property.type !== 'Identifier' || n.property.name !== 'enterFunction') {
                return;
            }

            n = n.object;
            if (!n || n.type !== 'MemberExpression') {
                return;
            }
            if (n.object.type !== 'Identifier' || n.object.name !== 'window') {
                return;
            }
            if (n.property.type !== 'Identifier' || n.property.name !== 'TRACE') {
                return;
            }

            traces.push({ start: parent[0].range[0], end: parent[0].range[1] });
        });

        // Remove the farther trace first, this is to avoid changing the range
        // info for each trace node.
        for (i = traces.length - 1; i >= 0; i -= 1) {
            trace = traces[i];
            if (code[trace.end] === '\n') {
                trace.end = trace.end + 1;
            }
            code = code.slice(0, trace.start) + code.slice(trace.end, code.length);
        }

        return code;
    }

    global.traceInstrument = function () {
        var tracer, code, i, functionList, signature, pos;

        if (typeof window.editor === 'undefined') {
            code = document.getElementById('code').value;
        } else {
            code = window.editor.getValue();
        }

        code = untrace(code);

        tracer = window.esmorph.Tracer.FunctionEntrance(function (fn) {
            signature = 'window.TRACE.enterFunction({ ';
            signature += 'name: "' + fn.name + '", ';
            signature += 'lineNumber: ' + fn.loc.start.line + ', ';
            signature += 'range: [' + fn.range[0] + ',' + fn.range[1] + ']';
            signature += ' });';
            return signature;
        });

        code = window.esmorph.modify(code, tracer);

        if (typeof window.editor === 'undefined') {
            document.getElementById('code').value = code;
        } else {
            window.editor.setValue(code);
        }

        // Enclose in IIFE.
        code = '(function() {\n' + code + '\n}())';

        return code;
    };

    function showResult() {
        var i, str, histogram, entry;

        histogram = window.TRACE.getHistogram();

        str = '<table><thead><tr><td>Function</td><td>Hits</td></tr></thead>';
        for (i = 0; i < histogram.length; i += 1) {
            entry = histogram[i];
            str += '<tr>';
            str += '<td>' + entry.name + '</td>';
            str += '<td>' + entry.count + '</td>';
            str += '</tr>';
        }
        str += '</table>';

        id('result').innerHTML = str;
    }

    function createTraceCollector() {
        global.TRACE = {
            hits: {},
            enterFunction: function (info) {
                var key = info.name + ' at line ' + info.lineNumber;
                if (this.hits.hasOwnProperty(key)) {
                    this.hits[key] = this.hits[key] + 1;
                } else {
                    this.hits[key] = 1;
                }
            },
            getHistogram: function () {
                var entry,
                    sorted = [];
                for (entry in this.hits) {
                    if (this.hits.hasOwnProperty(entry)) {
                        sorted.push({ name: entry, count: this.hits[entry]});
                    }
                }
                sorted.sort(function (a, b) {
                    return b.count - a.count;
                });
                return sorted;
            }
        };
    }

    global.traceRun = function () {
        var code;
        try {
            code = global.traceInstrument();
            createTraceCollector();
            eval(code);
            showResult();
        } catch (e) {
            id('result').innerText = e.toString();
        }
    };
}(window));
/* vim: set sw=4 ts=4 et tw=80 : */
