/*
 Copyright 2013 Daniel Wirtz <dcode@dcode.io>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/**
 * @license ascli (c) 2013 Daniel Wirtz <dcode@dcode.io>
 * Released under the Apache License, Version 2.0
 * see: https://github.com/dcodeIO/ascli for details
 */
module.exports = (function() {
    
    var util = require("util"),
        path = require("path"),
        colour = require("colour");
    
    // Enforce one blank before each line
    function indent1() {
        this.write(" "+util.format.apply(null, arguments).replace(/\n/g, "\n ")+"\n");
    }
    console.log = indent1.bind(process.stdout);
    console.info = indent1.bind(process.stdout);
    console.warn = indent1.bind(process.stderr);
    console.error = indent1.bind(process.stderr);
    
    // Default alphabet
    var alphabet = require(path.join(__dirname, "alphabet", "straight.json"));
    
    /**
     * Builds a banner.
     * @param {string=} title App name
     * @param {string=} appendix Appendix, e.g. version
     * @returns {string}
     */
    function ascli(title, appendix) {
        title = title || ascli.appName;
        appendix = appendix || "";
        var lines = ["", "", ""], c, a, j, ac = "";
        for (var i=0; i<title.length; i++) {
            c = title.charAt(i);
            if (c == '\x1B') {
                while ((c=title.charAt(i)) != 'm') {
                    ac += c;
                    i++;
                }
                ac += c;
            } else if ((a=alphabet[c])||(a=alphabet[c.toLowerCase()]))
                for (j=0; j<3; j++) 
                    lines[j] += ac+a[j];
        }
        for (i=0; i<lines.length; i++) lines[i] = lines[i]+"\x1B[0m";
        lines[1] += " "+appendix;
        if (lines[lines.length-1].strip.trim().length == 0) {
            lines.pop();
        }
        return '\n'+lines.join('\n')+'\n';
    }

    /**
     * App name.
     * @type {string}
     */
    ascli.appName = "app";

    /**
     * Prints a banner to console.
     * @param {string=} title Title in dojo alphabet
     * @param {string=} appendix Title appendix
     * @returns {Function} ascli
     */
    ascli.banner = function(title, appendix) {
        console.log(ascli(title, appendix));
        return ascli;
    };

    /**
     * Uses another alphabet.
     * @param {string|Object.<string,Array.<string>} alpha File name or alphabet to use
     * @returns {Function} ascli
     */
    ascli.use = function(alpha) {
        if (typeof alpha === 'string') {
            alphabet = require(alpha);
        } else {
            alphabet = alpha;
        }
        return ascli;
    };

    /**
     * Sets the app name.
     * @param {string} appName
     * @returns {Function} ascli
     */
    ascli.app = function(appName) {
        ascli.appName = appName;
        return ascli;
    };

    /**
     * Terminates the application with OK.
     * @param {string} msg Message text
     * @param {number=} code Exit code, defaults to 0
     */
    ascli.ok = function(msg, code) {
        process.stderr.write('\n '+ascli.appName.green.bold+' OK'.white.bold+(msg ? ' '+msg : '')+'\n');
        process.exit(typeof code != 'undefined' ? code : 0);
    };

    /**
     * Terminates the application with an ERROR.
     * @param {string} msg Message text
     * @param {number=} code Exit code
     */
    ascli.fail = function(msg, code) {
        process.stderr.write('\n '+ascli.appName.red.bold+' ERROR'.white.bold+(msg ? ' '+msg : '')+'\n');
        process.exit(typeof code != 'undefined' ? code : 1);
    };

    /**
     * opt.js
     * @param {Array.<string>=} argv
     * @returns {{node: string, script: string, argv: Array.<string>, opt: Object.<string,boolean|string>}}
     */
    ascli.optjs = require("optjs");
    
    // Pre-run it
    var opt = ascli.optjs();
    ascli.node = opt.node;
    ascli.script = opt.script;
    ascli.argv = opt.argv;
    ascli.opt = opt.opt;
    
    // Expose colour.js
    ascli.colour = ascli.colors = colour;
    
    return ascli;
    
})();
