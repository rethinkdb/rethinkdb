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
 * ProtoBuf.js Build Script (c) 2013 Daniel Wirtz <dcode@dcode.io>
 * Released under the Apache License, Version 2.0
 * see: https://github.com/dcodeIO/ProtoBuf.js for details
 */

var Preprocessor = require("preprocessor"),
    fs = require("fs");

var pkg = require(__dirname+"/package.json");

// Full build
var source = new Preprocessor(fs.readFileSync(__dirname+"/src/ProtoBuf.js"), __dirname+"/src").process({
    "NOPARSE": false,
    "VERSION": pkg.version
});
console.log("Writing ProtoBuf.js: "+source.length+" bytes");
fs.writeFileSync(__dirname+"/ProtoBuf.js", source);

// Noparse build
source = new Preprocessor(fs.readFileSync(__dirname+"/src/ProtoBuf.js"), __dirname+"/src").process({
    "NOPARSE": true,
    "VERSION": pkg.version
});
console.log("Writing ProtoBuf.noparse.js: "+source.length+" bytes");
fs.writeFileSync(__dirname+"/ProtoBuf.noparse.js", source);

// Bower versioning
source = new Preprocessor(fs.readFileSync(__dirname+"/src/bower.json"), __dirname+"/src").process({
    "VERSION": pkg.version
});
console.log("Writing bower.json: "+source.length+" bytes");
fs.writeFileSync(__dirname+"/bower.json", source);
