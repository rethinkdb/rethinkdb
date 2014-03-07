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
/*global esprima:true, harmonizr:true */

var updateId;

function updateCode(delay) {
    'use strict';

    function setText(id, str) {
        var el = document.getElementById(id);
        if (typeof el.innerText === 'string') {
            el.innerText = str;
        } else {
            el.textContent = str;
        }
    }

    setText('error', 'No error.');

    if (updateId) {
        window.clearTimeout(updateId);
    }

    updateId = window.setTimeout(function () {
        var code, result, timestamp;


        if (typeof window.editor === 'undefined') {
            code = document.getElementById('code').value;
        } else {
            code = window.editor.getValue();
        }

        try {
            timestamp = new Date();
            result = harmonizr.harmonize(code, { style: 'revealing' });
            timestamp = new Date() - timestamp;
            setText('error', 'No error. Transpiled in ' + timestamp + ' ms.');

            if (typeof window.resultview === 'undefined') {
                document.getElementById('result').value = result;
            } else {
                window.resultview.setValue(result);
            }
        } catch (e) {
            setText('error', e.name + ': ' + e.message);
        }

        updateId = undefined;
    }, delay || 250);
}

/*jslint sloppy:true browser:true */
/*global updateCode:true, CodeMirror:true */
window.onload = function () {
    var el;

    el = document.getElementById('version');
    if (typeof el.innerText === 'string') {
        el.innerText = window.esprima.version;
    } else {
        el.textContent = window.esprima.version;
    }

    try {
        updateCode(1);
    } catch (e) { }
};

try {
    window.checkEnv();

    // This is just testing, to detect whether CodeMirror would fail or not
    window.editor = CodeMirror.fromTextArea(document.getElementById("test"));

    window.editor = CodeMirror.fromTextArea(document.getElementById("code"), {
        lineNumbers: true,
        matchBrackets: true,
        onChange: updateCode
    });

    window.resultview = CodeMirror.fromTextArea(document.getElementById("result"), {
        lineNumbers: true,
        readOnly: true
    });
} catch (e) {
    // CodeMirror failed to initialize, possible in e.g. old IE.
    document.getElementById('codemirror').innerHTML = '';
} finally {
    document.getElementById('testbox').parentNode.removeChild(document.getElementById('testbox'));
}

