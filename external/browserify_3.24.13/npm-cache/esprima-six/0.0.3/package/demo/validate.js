/*jslint sloppy:true browser:true */
/*global esprima:true, CodeMirror:true */
var validateId;

function validate(delay) {
    if (validateId) {
        window.clearTimeout(validateId);
    }

    validateId = window.setTimeout(function () {
        var code, result, i, str;

        if (typeof window.editor === 'undefined') {
            code = document.getElementById('code').value;
        } else {
            code = window.editor.getValue();
        }

        try {
            result = esprima.parse(code, { tolerant: true, loc: true }).errors;
            if (result.length > 0) {
                str = '<p>Found <b>' + result.length + '</b>:</p>';
                for (i = 0; i < result.length; i += 1) {
                    str += '<p>' + result[i].message + '</p>';
                }
            } else {
                str = '<p>No syntax error.</p>';
            }
        } catch (e) {
            str = e.name + ': ' + e.message;
        }
        document.getElementById('result').innerHTML = str;

        validateId = undefined;
    }, delay || 811);
}

window.onload = function () {
    var id, el;

    id = function (i) {
        return document.getElementById(i);
    };

    el = id('version');
    if (typeof el.innerText === 'string') {
        el.innerText = esprima.version;
    } else {
        el.textContent = esprima.version;
    }
    try {
        validate(1);
    } catch (e) { }
};

try {
    window.checkEnv();

    // This is just testing, to detect whether CodeMirror would fail or not
    window.editor = CodeMirror.fromTextArea(document.getElementById("test"));

    window.editor = CodeMirror.fromTextArea(document.getElementById("code"), {
        lineNumbers: true,
        matchBrackets: true,
        onChange: validate
    });
} catch (e) {
    // CodeMirror failed to initialize, possible in e.g. old IE.
    document.getElementById('codemirror').innerHTML = '';
    document.getElementById('code').onchange = validate;
    document.getElementById('code').onkeydown = validate;
} finally {
    document.getElementById('testbox').parentNode.removeChild(document.getElementById('testbox'));
}
