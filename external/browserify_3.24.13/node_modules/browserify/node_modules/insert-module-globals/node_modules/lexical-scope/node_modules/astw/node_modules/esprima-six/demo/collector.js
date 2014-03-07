/*
  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

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

/*jslint browser:true */

var timerId;

function collectRegex() {
    'use strict';

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

    function process(delay) {
        if (timerId) {
            window.clearTimeout(timerId);
        }

        timerId = window.setTimeout(function () {
            var code, result, i, str;

            if (typeof window.editor === 'undefined') {
                code = document.getElementById('code').value;
            } else {
                code = window.editor.getValue();
            }

            // Executes f on the object and its children (recursively).
            function visit(object, f) {
                var key, child;

                if (f.call(null, object) === false) {
                    return;
                }
                for (key in object) {
                    if (object.hasOwnProperty(key)) {
                        child = object[key];
                        if (typeof child === 'object' && child !== null) {
                            visit(child, f);
                        }
                    }
                }
            }

            function createRegex(pattern, mode) {
                var literal;
                try {
                    literal = new RegExp(pattern, mode);
                } catch (e) {
                    // Invalid regular expression.
                    return;
                }
                return literal;
            }

            function collect(node) {
                var str, arg, value;
                if (node.type === 'Literal') {
                    if (node.value instanceof RegExp) {
                        str = node.value.toString();
                        if (str[0] === '/') {
                            result.push({
                                type: 'Literal',
                                value: node.value,
                                line: node.loc.start.line,
                                column: node.loc.start.column
                            });
                        }
                    }
                }
                if (node.type === 'NewExpression' || node.type === 'CallExpression') {
                    if (node.callee.type === 'Identifier' && node.callee.name === 'RegExp') {
                        arg = node['arguments'];
                        if (arg.length === 1 && arg[0].type === 'Literal') {
                            if (typeof arg[0].value === 'string') {
                                value = createRegex(arg[0].value);
                                if (value) {
                                    result.push({
                                        type: 'Literal',
                                        value: value,
                                        line: node.loc.start.line,
                                        column: node.loc.start.column
                                    });
                                }
                            }
                        }
                        if (arg.length === 2 && arg[0].type === 'Literal' && arg[1].type === 'Literal') {
                            if (typeof arg[0].value === 'string' && typeof arg[1].value === 'string') {
                                value = createRegex(arg[0].value, arg[1].value);
                                if (value) {
                                    result.push({
                                        type: 'Literal',
                                        value: value,
                                        line: node.loc.start.line,
                                        column: node.loc.start.column
                                    });
                                }
                            }
                        }
                    }
                }
            }

            try {
                result = [];
                visit(window.esprima.parse(code, { loc: true }), collect);

                if (result.length > 0) {
                    str = '<p>Found <b>' + result.length + '</b> regex(s):</p>';
                    for (i = 0; i < result.length; i += 1) {
                        str += '<p>' + 'Line ' + result[i].line;
                        str += ' column ' + (1 + result[i].column);
                        str += ': <code>'; str += escaped(result[i].value.toString()) + '</code>';
                        str += '</p>';
                    }
                    id('result').innerHTML = str;
                } else {
                    setText('result', 'No regex.');
                }
            } catch (e) {
                setText('result', e.toString());
            }

            timerId = undefined;
        }, delay || 811);
    }

    setText('version', window.esprima.version);
    process(1);
}
/* vim: set sw=4 ts=4 et tw=80 : */
