mine.js
=======

The mine script accepts js source and returns all the require call locations as well as the target string. This is a submodule of [js-linker](https://github.com/creationix/js-linker).

``` js
var mine = require('js-linker/mine.js');
var fs = require('fs');
var code = fs.readFileSync("test.js");
var deps = mine(code);
```