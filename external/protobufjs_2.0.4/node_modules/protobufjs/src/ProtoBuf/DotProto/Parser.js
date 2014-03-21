// #ifdef UNDEFINED
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
// #endif
/**
 * @alias ProtoBuf.DotProto.Parser
 * @expose
 */
ProtoBuf.DotProto.Parser = (function(ProtoBuf, Lang, Tokenizer) {
    "use strict";
    
    /**
     * Constructs a new Parser.
     * @exports ProtoBuf.DotProto.Parser
     * @class A ProtoBuf .proto parser.
     * @param {string} proto Protocol source
     * @constructor
     */
    var Parser = function(proto) {

        /**
         * Tokenizer.
         * @type {ProtoBuf.DotProto.Tokenizer}
         * @expose
         */
        this.tn = new Tokenizer(proto);
    };

    /**
     * Runs the parser.
     * @return {{package: string|null, messages: Array.<object>, enums: Array.<object>, imports: Array.<string>, options: object<string,*>}}
     * @throws {Error} If the source cannot be parsed
     * @expose
     */
    Parser.prototype.parse = function() {
        var topLevel = {
            "name": "[ROOT]", // temporary
            "package": null,
            "messages": [],
            "enums": [],
            "imports": [],
            "options": {},
            "services": []
        };
        var token, header = true;
        do {
            token = this.tn.next();
            if (token == null) {
                break; // No more messages
            }
            if (token == 'package') {
                if (!header) {
                    throw(new Error("Illegal package definition at line "+this.tn.line+": Must be declared before the first message or enum"));
                }
                if (topLevel["package"] !== null) {
                    throw(new Error("Illegal package definition at line "+this.tn.line+": Package already declared"));
                }
                topLevel["package"] = this._parsePackage(token);
            } else if (token == 'import') {
                if (!header) {
                    throw(new Error("Illegal import definition at line "+this.tn.line+": Must be declared before the first message or enum"));
                }
                topLevel.imports.push(this._parseImport(token));
            } else if (token === 'message') {
                this._parseMessage(topLevel, token);
                header = false;
            } else if (token === 'enum') {
                this._parseEnum(topLevel, token);
                header = false;
            } else if (token === 'option') {
                if (!header) {
                    throw(new Error("Illegal option definition at line "+this.tn.line+": Must be declared before the first message or enum"));
                }
                this._parseOption(topLevel, token);
            } else if (token === 'service') {
                this._parseService(topLevel, token);
            } else if (token === 'extend') {
                this._parseExtend(topLevel, token);
            } else if (token === 'syntax') {
                this._parseIgnoredStatement(topLevel, token);
            } else {
                throw(new Error("Illegal top level declaration at line "+this.tn.line+": "+token));
            }
        } while (true);
        delete topLevel["name"];
        return topLevel;
    };

    /**
     * Parses a number value.
     * @param {string} val Number value to parse
     * @return {number} Number
     * @throws {Error} If the number value is invalid
     * @private
     */
    Parser.prototype._parseNumber = function(val) {
        var sign = 1;
        if (val.charAt(0) == '-') {
            sign = -1; val = val.substring(1);
        }
        if (Lang.NUMBER_DEC.test(val)) {
            return sign*parseInt(val, 10);
        } else if (Lang.NUMBER_HEX.test(val)) {
            return sign*parseInt(val.substring(2), 16);
        } else if (Lang.NUMBER_OCT.test(val)) {
            return sign*parseInt(val.substring(1), 8);
        } else if (Lang.NUMBER_FLT.test(val)) {
            return sign*parseFloat(val);
        }
        throw(new Error("Illegal number value at line "+this.tn.line+": "+(sign < 0 ? '-' : '')+val));
    };

    /**
     * Parses an ID value.
     * @param {string} val ID value to parse
     * @param {boolean=} neg Whether the ID may be negative, defaults to `false`
     * @returns {number} ID
     * @throws {Error} If the ID value is invalid
     * @private
     */
    Parser.prototype._parseId = function(val, neg) {
        var id = -1;
        var sign = 1;
        if (val.charAt(0) == '-') {
            sign = -1; val = val.substring(1);
        }
        if (Lang.NUMBER_DEC.test(val)) {
            id = parseInt(val);
        } else if (Lang.NUMBER_HEX.test(val)) {
            id = parseInt(val.substring(2), 16);
        } else if (Lang.NUMBER_OCT.test(val)) {
            id = parseInt(val.substring(1), 8);
        } else {
            throw(new Error("Illegal ID value at line "+this.tn.line+": "+(sign < 0 ? '-' : '')+val));
        }
        id = (sign*id)|0; // Force to 32bit
        if (!neg && id < 0) {
            throw(new Error("Illegal ID range at line "+this.tn.line+": "+(sign < 0 ? '-' : '')+val));
        }
        return id;
    };

    /**
     * Parses the package definition.
     * @param {string} token Initial token
     * @return {string} Package name
     * @throws {Error} If the package definition cannot be parsed
     * @private
     */
    Parser.prototype._parsePackage = function(token) {
        token = this.tn.next();
        if (!Lang.TYPEREF.test(token)) {
            throw(new Error("Illegal package name at line "+this.tn.line+": "+token));
        }
        var pkg = token;
        token = this.tn.next();
        if (token != Lang.END) {
            throw(new Error("Illegal end of package definition at line "+this.tn.line+": "+token+" ('"+Lang.END+"' expected)"));
        }
        return pkg;
    };

    /**
     * Parses an import definition.
     * @param {string} token Initial token
     * @return {string} Import file name 
     * @throws {Error} If the import definition cannot be parsed
     * @private
     */
    Parser.prototype._parseImport = function(token) {
        token = this.tn.next();
        if (token === "public") {
            token = this.tn.next();
        }
        if (token !== Lang.STRINGOPEN) {
            throw(new Error("Illegal begin of import value at line "+this.tn.line+": "+token+" ('"+Lang.STRINGOPEN+"' expected)"));
        }
        var imported = this.tn.next();
        token = this.tn.next();
        if (token !== Lang.STRINGCLOSE) {
            throw(new Error("Illegal end of import value at line "+this.tn.line+": "+token+" ('"+Lang.STRINGCLOSE+"' expected)"));
        }
        token = this.tn.next();
        if (token !== Lang.END) {
            throw(new Error("Illegal end of import definition at line "+this.tn.line+": "+token+" ('"+Lang.END+"' expected)"));
        }
        return imported;
    };

    /**
     * Parses a namespace option.
     * @param {Object} parent Parent definition
     * @param {string} token Initial token
     * @throws {Error} If the option cannot be parsed
     * @private
     */
    Parser.prototype._parseOption = function(parent, token) {
        token = this.tn.next();
        var custom = false;
        if (token == Lang.COPTOPEN) {
            custom = true;
            token = this.tn.next();
        }
        if (!Lang.NAME.test(token)) {
            // we can allow options of the form google.protobuf.* since they will just get ignored anyways
            if (!/google\.protobuf\./.test(token)) {
                throw(new Error("Illegal option name in message "+parent.name+" at line "+this.tn.line+": "+token));
            }
        }
        var name = token;
        token = this.tn.next();
        if (custom) { // (my_method_option).foo, (my_method_option), some_method_option
            if (token !== Lang.COPTCLOSE) {
                throw(new Error("Illegal custom option name delimiter in message "+parent.name+", option "+name+" at line "+this.tn.line+": "+token+" ('"+Lang.COPTCLOSE+"' expected)"));
            }
            name = '('+name+')';
            token = this.tn.next();
            if (Lang.FQTYPEREF.test(token)) {
                name += token;
                token = this.tn.next();
            }
        }
        if (token !== Lang.EQUAL) {
            throw(new Error("Illegal option operator in message "+parent.name+", option "+name+" at line "+this.tn.line+": "+token+" ('"+Lang.EQUAL+"' expected)"));
        }
        var value;
        token = this.tn.next();
        if (token === Lang.STRINGOPEN) {
            value = this.tn.next();
            token = this.tn.next();
            if (token !== Lang.STRINGCLOSE) {
                throw(new Error("Illegal end of option value in message "+parent.name+", option "+name+" at line "+this.tn.line+": "+token+" ('"+Lang.STRINGCLOSE+"' expected)"));
            }
        } else {
            if (Lang.NUMBER.test(token)) {
                value = this._parseNumber(token, true);
            } else if (Lang.TYPEREF.test(token)) {
                value = token;
            } else {
                throw(new Error("Illegal option value in message "+parent.name+", option "+name+" at line "+this.tn.line+": "+token));
            }
        }
        token = this.tn.next();
        if (token !== Lang.END) {
            throw(new Error("Illegal end of option in message "+parent.name+", option "+name+" at line "+this.tn.line+": "+token+" ('"+Lang.END+"' expected)"));
        }
        parent["options"][name] = value;
    };

    /**
     * Parses an ignored block of the form ['keyword', 'typeref', '{' ... '}'].
     * @param {Object} parent Parent definition
     * @param {string} keyword Initial token
     * @throws {Error} If the directive cannot be parsed
     * @private
     */
    Parser.prototype._parseIgnoredBlock = function(parent, keyword) {
        var token = this.tn.next();
        if (!Lang.TYPEREF.test(token)) {
            throw(new Error("Illegal "+keyword+" type in "+parent.name+": "+token));
        }
        var name = token;
        token = this.tn.next();
        if (token !== Lang.OPEN) {
            throw(new Error("Illegal OPEN in "+parent.name+" after "+keyword+" "+name+" at line "+this.tn.line+": "+token));
        }
        var depth = 1;
        do {
            token = this.tn.next();
            if (token === null) {
                throw(new Error("Unexpected EOF in "+parent.name+", "+keyword+" (ignored) at line "+this.tn.line+": "+name));
            }
            if (token === Lang.OPEN) {
                depth++;
            } else if (token === Lang.CLOSE) {
                token = this.tn.peek();
                if (token === Lang.END) this.tn.next();
                depth--;
                if (depth === 0) {
                    break;
                }
            }
        } while(true);
    };

    /**
     * Parses an ignored statement of the form ['keyword', ..., ';'].
     * @param {Object} parent Parent definition
     * @param {string} keyword Initial token
     * @throws {Error} If the directive cannot be parsed
     * @private
     */
    Parser.prototype._parseIgnoredStatement = function(parent, keyword) {
        var token;
        do {
            token = this.tn.next();
            if (token === null) {
                throw(new Error("Unexpected EOF in "+parent.name+", "+keyword+" (ignored) at line "+this.tn.line));
            }
            if (token === Lang.END) break;
        } while (true);
    };

    /**
     * Parses a service definition.
     * @param {Object} parent Parent definition
     * @param {string} keyword Initial token
     * @throws {Error} If the service cannot be parsed
     * @private
     */
    Parser.prototype._parseService = function(parent, keyword) {
        var token = this.tn.next();
        if (!Lang.NAME.test(token)) {
            throw(new Error("Illegal service name at line "+this.tn.line+": "+token));
        }
        var name = token;
        var svc = {
            "name": name,
            "rpc": {},
            "options": {}
        };
        token = this.tn.next();
        if (token !== Lang.OPEN) {
            throw(new Error("Illegal OPEN after service "+name+" at line "+this.tn.line+": "+token+" ('"+Lang.OPEN+"' expected)"));
        }
        do {
            token = this.tn.next();
            if (token === "option") {
                this._parseOption(svc, token);
            } else if (token === 'rpc') {
                this._parseServiceRPC(svc, token);
            } else if (token !== Lang.CLOSE) {
                throw(new Error("Illegal type for service "+name+" at line "+this.tn.line+": "+token));
            }
        } while (token !== Lang.CLOSE);
        parent["services"].push(svc);
    };

    /**
     * Parses a RPC service definition of the form ['rpc', name, (request), 'returns', (response)].
     * @param {Object} svc Parent definition
     * @param {string} token Initial token
     * @private
     */
    Parser.prototype._parseServiceRPC = function(svc, token) {
        var type = token;
        token = this.tn.next();
        if (!Lang.NAME.test(token)) {
            throw(new Error("Illegal RPC method name in service "+svc["name"]+" at line "+this.tn.line+": "+token));
        }
        var name = token;
        var method = {
            "request": null,
            "response": null,
            "options": {}
        };
        token = this.tn.next();
        if (token !== Lang.COPTOPEN) {
            throw(new Error("Illegal start of request type in RPC service "+svc["name"]+"#"+name+" at line "+this.tn.line+": "+token+" ('"+Lang.COPTOPEN+"' expected)"));
        }
        token = this.tn.next();
        if (!Lang.TYPEREF.test(token)) {
            throw(new Error("Illegal request type in RPC service "+svc["name"]+"#"+name+" at line "+this.tn.line+": "+token));
        }
        method["request"] = token;
        token = this.tn.next();
        if (token != Lang.COPTCLOSE) {
            throw(new Error("Illegal end of request type in RPC service "+svc["name"]+"#"+name+" at line "+this.tn.line+": "+token+" ('"+Lang.COPTCLOSE+"' expected)"))
        }
        token = this.tn.next();
        if (token.toLowerCase() !== "returns") {
            throw(new Error("Illegal request/response delimiter in RPC service "+svc["name"]+"#"+name+" at line "+this.tn.line+": "+token+" ('returns' expected)"));
        }
        token = this.tn.next();
        if (token != Lang.COPTOPEN) {
            throw(new Error("Illegal start of response type in RPC service "+svc["name"]+"#"+name+" at line "+this.tn.line+": "+token+" ('"+Lang.COPTOPEN+"' expected)"));
        }
        token = this.tn.next();
        method["response"] = token;
        token = this.tn.next();
        if (token !== Lang.COPTCLOSE) {
            throw(new Error("Illegal end of response type in RPC service "+svc["name"]+"#"+name+" at line "+this.tn.line+": "+token+" ('"+Lang.COPTCLOSE+"' expected)"))
        }
        token = this.tn.next();
        if (token === Lang.OPEN) {
            do {
                token = this.tn.next();
                if (token === 'option') {
                    this._parseOption(method, token); // <- will fail for the custom-options example
                } else if (token !== Lang.CLOSE) {
                    throw(new Error("Illegal start of option in RPC service "+svc["name"]+"#"+name+" at line "+this.tn.line+": "+token+" ('option' expected)"));
                }
            } while (token !== Lang.CLOSE);
        } else if (token !== Lang.END) {
            throw(new Error("Illegal method delimiter in RPC service "+svc["name"]+"#"+name+" at line "+this.tn.line+": "+token+" ('"+Lang.END+"' or '"+Lang.OPEN+"' expected)"));
        }
        if (typeof svc[type] === 'undefined') svc[type] = {};
        svc[type][name] = method;
    };

    /**
     * Parses a message definition.
     * @param {Object} parent Parent definition
     * @param {string} token First token
     * @return {Object}
     * @throws {Error} If the message cannot be parsed
     * @private
     */
    Parser.prototype._parseMessage = function(parent, token) {
        /** @dict */
        var msg = {}; // Note: At some point we might want to exclude the parser, so we need a dict.
        token = this.tn.next();
        if (!Lang.NAME.test(token)) {
            throw(new Error("Illegal message name"+(parent ? " in message "+parent["name"] : "")+" at line "+this.tn.line+": "+token));
        }
        msg["name"] = token;
        token = this.tn.next();
        if (token != Lang.OPEN) {
            throw(new Error("Illegal OPEN after message "+msg.name+" at line "+this.tn.line+": "+token+" ('"+Lang.OPEN+"' expected)"));
        }
        msg["fields"] = []; // Note: Using arrays to support also browser that cannot preserve order of object keys.
        msg["enums"] = [];
        msg["messages"] = [];
        msg["options"] = {};
        // msg["extensions"] = undefined
        do {
            token = this.tn.next();
            if (token === Lang.CLOSE) {
                token = this.tn.peek();
                if (token === Lang.END) this.tn.next();
                break;
            } else if (Lang.RULE.test(token)) {
                this._parseMessageField(msg, token);
            } else if (token === "enum") {
                this._parseEnum(msg, token);
            } else if (token === "message") {
                this._parseMessage(msg, token);
            } else if (token === "option") {
                this._parseOption(msg, token);
            } else if (token === "extensions") {
                msg["extensions"] = this._parseExtensions(msg, token);
            } else if (token === "extend") {
                this._parseExtend(msg, token);
            } else {
                throw(new Error("Illegal token in message "+msg.name+" at line "+this.tn.line+": "+token+" (type or '"+Lang.CLOSE+"' expected)"));
            }
        } while (true);
        parent["messages"].push(msg);
        return msg;
    };

    /**
     * Parses a message field.
     * @param {Object} msg Message definition
     * @param {string} token Initial token
     * @throws {Error} If the message field cannot be parsed
     * @private
     */
    Parser.prototype._parseMessageField = function(msg, token) {
        /** @dict */
        var fld = {};
        fld["rule"] = token;
        token = this.tn.next();
        if (!Lang.TYPE.test(token) && !Lang.TYPEREF.test(token)) {
            throw(new Error("Illegal field type in message "+msg.name+" at line "+this.tn.line+": "+token));
        }
        fld["type"] = token;
        token = this.tn.next();
        if (!Lang.NAME.test(token)) {
            throw(new Error("Illegal field name in message "+msg.name+" at line "+this.tn.line+": "+token));
        }
        fld["name"] = token;
        token = this.tn.next();
        if (token !== Lang.EQUAL) {
            throw(new Error("Illegal field number operator in message "+msg.name+"#"+fld.name+" at line "+this.tn.line+": "+token+" ('"+Lang.EQUAL+"' expected)"));
        }
        token = this.tn.next();
        try {
            fld["id"] = this._parseId(token);
        } catch (e) {
            throw(new Error("Illegal field id in message "+msg.name+"#"+fld.name+" at line "+this.tn.line+": "+token));
        }
        /** @dict */
        fld["options"] = {};
        token = this.tn.next();
        if (token === Lang.OPTOPEN) {
            this._parseFieldOptions(msg, fld, token);
            token = this.tn.next();
        }
        if (token !== Lang.END) {
            throw(new Error("Illegal field delimiter in message "+msg.name+"#"+fld.name+" at line "+this.tn.line+": "+token+" ('"+Lang.END+"' expected)"));
        }
        msg["fields"].push(fld);
    };

    /**
     * Parses a set of field option definitions.
     * @param {Object} msg Message definition
     * @param {Object} fld Field definition
     * @param {string} token Initial token
     * @throws {Error} If the message field options cannot be parsed
     * @private
     */
    Parser.prototype._parseFieldOptions = function(msg, fld, token) {
        var first = true;
        do {
            token = this.tn.next();
            if (token === Lang.OPTCLOSE) {
                break;
            } else if (token === Lang.OPTEND) {
                if (first) {
                    throw(new Error("Illegal start of message field options in message "+msg.name+"#"+fld.name+" at line "+this.tn.line+": "+token));
                }
                token = this.tn.next();
            }
            this._parseFieldOption(msg, fld, token);
            first = false;
        } while (true);
    };

    /**
     * Parses a single field option.
     * @param {Object} msg Message definition
     * @param {Object} fld Field definition
     * @param {string} token Initial token
     * @throws {Error} If the mesage field option cannot be parsed
     * @private
     */
    Parser.prototype._parseFieldOption = function(msg, fld, token) {
        var custom = false;
        if (token === Lang.COPTOPEN) {
            token = this.tn.next();
            custom = true;
        }
        if (!Lang.NAME.test(token)) {
            throw(new Error("Illegal field option in message "+msg.name+"#"+fld.name+" at line "+this.tn.line+": "+token));
        }
        var name = token;
        token = this.tn.next();
        if (custom) {
            if (token !== Lang.COPTCLOSE) {
                throw(new Error("Illegal custom field option name delimiter in message "+msg.name+"#"+fld.name+" at line "+this.tn.line+": "+token+" (')' expected)"));
            }
            name = '('+name+')';
            token = this.tn.next();
            if (Lang.FQTYPEREF.test(token)) {
                name += token;
                token = this.tn.next();
            }
        }
        if (token !== Lang.EQUAL) {
            throw(new Error("Illegal field option operation in message "+msg.name+"#"+fld.name+" at line "+this.tn.line+": "+token+" ('=' expected)"));
        }
        var value;
        token = this.tn.next();
        if (token === Lang.STRINGOPEN) {
            value = this.tn.next();
            token = this.tn.next();
            if (token != Lang.STRINGCLOSE) {
                throw(new Error("Illegal end of field value in message "+msg.name+"#"+fld.name+", option "+name+" at line "+this.tn.line+": "+token+" ('"+Lang.STRINGCLOSE+"' expected)"));
            }
        } else if (Lang.NUMBER.test(token, true)) {
            value = this._parseNumber(token, true);
        } else if (Lang.BOOL.test(token)) {
            value = token.toLowerCase() === 'true';
        } else if (Lang.TYPEREF.test(token)) {
            value = token; // TODO: Resolve?
        } else {
            throw(new Error("Illegal field option value in message "+msg.name+"#"+fld.name+", option "+name+" at line "+this.tn.line+": "+token));
        }
        fld["options"][name] = value;
    };

    /**
     * Parses an enum.
     * @param {Object} msg Message definition
     * @param {string} token Initial token
     * @throws {Error} If the enum cannot be parsed
     * @private
     */
    Parser.prototype._parseEnum = function(msg, token) {
        /** @dict */
        var enm = {};
        token = this.tn.next();
        if (!Lang.NAME.test(token)) {
            throw(new Error("Illegal enum name in message "+msg.name+" at line "+this.tn.line+": "+token));
        }
        enm["name"] = token;
        token = this.tn.next();
        if (token !== Lang.OPEN) {
            throw(new Error("Illegal OPEN after enum "+enm.name+" at line "+this.tn.line+": "+token));
        }
        enm["values"] = [];
        enm["options"] = {};
        do {
            token = this.tn.next();
            if (token === Lang.CLOSE) {
                token = this.tn.peek();
                if (token === Lang.END) this.tn.next();
                break;
            }
            if (token == 'option') {
                this._parseOption(enm, token);
            } else {
                if (!Lang.NAME.test(token)) {
                    throw(new Error("Illegal enum value name in enum "+enm.name+" at line "+this.tn.line+": "+token));
                }
                this._parseEnumValue(enm, token);
            }
        } while (true);
        msg["enums"].push(enm);
    };

    /**
     * Parses an enum value.
     * @param {Object} enm Enum definition
     * @param {string} token Initial token
     * @throws {Error} If the enum value cannot be parsed
     * @private
     */
    Parser.prototype._parseEnumValue = function(enm, token) {
        /** @dict */
        var val = {};
        val["name"] = token;
        token = this.tn.next();
        if (token !== Lang.EQUAL) {
            throw(new Error("Illegal enum value operator in enum "+enm.name+" at line "+this.tn.line+": "+token+" ('"+Lang.EQUAL+"' expected)"));
        }
        token = this.tn.next();
        try {
            val["id"] = this._parseId(token, true);
        } catch (e) {
            throw(new Error("Illegal enum value id in enum "+enm.name+" at line "+this.tn.line+": "+token));
        }
        enm["values"].push(val);
        token = this.tn.next();
        if (token === Lang.OPTOPEN) {
            var opt = { 'options' : {} }; // TODO: Actually expose them somehow.
            this._parseFieldOptions(enm, opt, token);
            token = this.tn.next();
        }
        if (token !== Lang.END) {
            throw(new Error("Illegal enum value delimiter in enum "+enm.name+" at line "+this.tn.line+": "+token+" ('"+Lang.END+"' expected)"));
        }
    };

    /**
     * Parses an extensions statement.
     * @param {Object} msg Message object
     * @param {string} token Initial token
     * @throws {Error} If the extensions statement cannot be parsed
     * @private
     */
    Parser.prototype._parseExtensions = function(msg, token) {
        /** @type {Array.<number>} */
        var range = [];
        token = this.tn.next();
        if (token === "min") { // FIXME: Does the official implementation support this?
            range.push(Lang.ID_MIN);
        } else if (token === "max") {
            range.push(Lang.ID_MAX);
        } else {
            range.push(this._parseNumber(token));
        }
        token = this.tn.next();
        if (token !== 'to') {
            throw("Illegal extensions delimiter in message "+msg.name+" at line "+this.tn.line+" ('to' expected)");
        }
        token = this.tn.next();
        if (token === "min") {
            range.push(Lang.ID_MIN);
        } else if (token === "max") {
            range.push(Lang.ID_MAX);
        } else {
            range.push(this._parseNumber(token));
        }
        token = this.tn.next();
        if (token !== Lang.END) {
            throw(new Error("Illegal extension delimiter in message "+msg.name+" at line "+this.tn.line+": "+token+" ('"+Lang.END+"' expected)"));
        }
        return range;
    };

    /**
     * Parses an extend block.
     * @param {Object} parent Parent object
     * @param {string} token Initial token
     * @throws {Error} If the extend block cannot be parsed
     * @private
     */
    Parser.prototype._parseExtend = function(parent, token) {
        token = this.tn.next();
        if (!Lang.TYPEREF.test(token)) {
            throw(new Error("Illegal extended message name at line "+this.tn.line+": "+token));
        }
        /** @dict */
        var ext = {};
        ext["ref"] = token;
        ext["fields"] = [];
        token = this.tn.next();
        if (token !== Lang.OPEN) {
            throw(new Error("Illegal OPEN in extend "+ext.name+" at line "+this.tn.line+": "+token+" ('"+Lang.OPEN+"' expected)"));
        }
        do {
            token = this.tn.next();
            if (token === Lang.CLOSE) {
                token = this.tn.peek();
                if (token == Lang.END) this.tn.next();
                break;
            } else if (Lang.RULE.test(token)) {
                this._parseMessageField(ext, token);
            } else {
                throw(new Error("Illegal token in extend "+ext.name+" at line "+this.tn.line+": "+token+" (rule or '"+Lang.CLOSE+"' expected)"));
            }
        } while (true);
        parent["messages"].push(ext);
        return ext;
    };

    /**
     * Returns a string representation of this object.
     * @returns {string} String representation as of "Parser"
     */
    Parser.prototype.toString = function() {
        return "Parser";
    };
    
    return Parser;
    
})(ProtoBuf, ProtoBuf.Lang, ProtoBuf.DotProto.Tokenizer);
