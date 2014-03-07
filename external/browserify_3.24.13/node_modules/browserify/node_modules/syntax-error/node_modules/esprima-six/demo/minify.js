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

/*jslint browser:true */
/*global esprima:true, escodegen:true */

function id(i) {
    'use strict';
    return document.getElementById(i);
}

function setText(id, str) {
    'use strict';
    var el = document.getElementById(id);
    if (typeof el.innerText === 'string') {
        el.innerText = str;
    } else {
        el.textContent = str;
    }
}

function minify() {
    'use strict';

    var code, syntax, option, before, after;

    setText('error', '');
    if (typeof window.editor !== 'undefined') {
        // Using CodeMirror.
        code = window.editor.getValue();
    } else {
        // Plain textarea, likely in a situation where CodeMirror does not work.
        code = id('code').value;
    }

    option = {
        format: {
            indent: {
                style: ''
            },
            quotes: 'auto',
            compact: true
        }
    };

    try {
        before = code.length;
        syntax = window.esprima.parse(code, { raw: true, range: true });
        code = window.escodegen.generate(syntax, option);
        if (typeof window.editor !== 'undefined') {
            window.editor.setValue(code);
        } else {
            id('code').value = code;
        }
        after = code.length;
        if (before > after) {
            setText('error', 'No error. Minifying ' + before + ' bytes to ' + after + ' bytes.');
        } else {
            setText('error', 'Can not minify further, code is already optimized.');
        }
    } catch (e) {
        setText('error', e.toString());
    } finally {
    }
}

/*jslint sloppy:true browser:true */
/*global minify:true, CodeMirror:true */
window.onload = function () {
    var version, el;

    version = 'Using Esprima version ' + esprima.version;
    version += ' and Escodegen version ' + escodegen.version + '.';

    el = id('version');
    if (typeof el.innerText === 'string') {
        el.innerText = version;
    } else {
        el.textContent = version;
    }

    id('minify').onclick = minify;

    try {
        window.checkEnv();

        // This is just testing, to detect whether CodeMirror would fail or not
        window.editor = CodeMirror.fromTextArea(id("test"));

        window.editor = CodeMirror.fromTextArea(id("code"), {
            lineNumbers: true,
            matchBrackets: true,
            lineWrapping: true
        });
    } catch (e) {
        // CodeMirror failed to initialize, possible in e.g. old IE.
        id('codemirror').innerHTML = '';
    } finally {
        id('testbox').parentNode.removeChild(id('testbox'));
    }
};
