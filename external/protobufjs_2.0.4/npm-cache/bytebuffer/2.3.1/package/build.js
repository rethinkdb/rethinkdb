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
 * ByteBuffer.js Build Script (c) 2013 Daniel Wirtz <dcode@dcode.io>
 * Released under the Apache License, Version 2.0
 * see: https://github.com/dcodeIO/ByteBuffer.js for details
 */

var Preprocessor = require("preprocessor"),
    fs = require("fs"),
    pkg = require(__dirname+"/package.json");

var pp = new Preprocessor(fs.readFileSync(__dirname+"/src/ByteBuffer.js"), __dirname+"/src");
fs.writeFileSync(__dirname+"/ByteBuffer.js", pp.process({
    "VERSION": pkg.version
}));

pp = new Preprocessor(fs.readFileSync(__dirname+"/src/bower.json"), __dirname+"/src");
fs.writeFileSync(__dirname+"/bower.json", pp.process({
    "VERSION": pkg.version
}));
