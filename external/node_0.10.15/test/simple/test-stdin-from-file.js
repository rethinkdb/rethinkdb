// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.




var common = require('../common');
var assert = require('assert');
var join = require('path').join;
var childProccess = require('child_process');
var fs = require('fs');

var stdoutScript = join(common.fixturesDir, 'echo.js');
var tmpFile = join(common.fixturesDir, 'stdin.txt');

var cmd = '"' + process.argv[0] + '" "' + stdoutScript + '" < "' +
    tmpFile + '"';

var string = 'abc\nümlaut.\nsomething else\n' +
             '南越国是前203年至前111年存在于岭南地区的一个国家，国都位于番禺，' +
             '疆域包括今天中国的广东、广西两省区的大部份地区，福建省、湖南、贵州、' +
             '云南的一小部份地区和越南的北部。南越国是秦朝灭亡后，' +
             '由南海郡尉赵佗于前203年起兵兼并桂林郡和象郡后建立。前196年和前179年，' +
             '南越国曾先后两次名义上臣属于西汉，成为西汉的“外臣”。前112年，' +
             '南越国末代君主赵建德与西汉发生战争，被汉武帝于前111年所灭。南越国共存在93年，' +
             '历经五代君主。南越国是岭南地区的第一个有记载的政权国家，' +
             '采用封建制和郡县制并存的制度，' +
             '它的建立保证了秦末乱世岭南地区社会秩序的稳定，' +
             '有效的改善了岭南地区落后的政治、##济现状。\n';


console.log(cmd + '\n\n');

try {
  fs.unlinkSync(tmpFile);
} catch (e) {}

fs.writeFileSync(tmpFile, string);

childProccess.exec(cmd, function(err, stdout, stderr) {
  fs.unlinkSync(tmpFile);

  if (err) throw err;
  console.log(stdout);
  assert.equal(stdout, 'hello world\r\n' + string);
  assert.equal('', stderr);
});
