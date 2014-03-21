![opt.js](https://raw.github.com/dcodeIO/opt.js/master/opt.png)
======
Probably the sole command line option parser you'll ever need to <del>`npm install optjs`</del> Ctrl+C, Ctrl+V. Proof:

```js
function opt(argv) {
    var opt={},arg,p;argv=Array.prototype.slice.call(argv||process.argv);for(var i=2;i<argv.length;i++)if(argv[i].charAt(0)=='-')
    ((p=(arg=(""+argv.splice(i--,1)).replace(/^[\-]+/,'')).indexOf("="))>0?opt[arg.substring(0,p)]=arg.substring(p+1):opt[arg]=true);
    return {'node':argv[0],'script':argv[1],'argv':argv.slice(2),'opt':opt};
}
```

Usage
-----
```js
var opt = require("optjs")();
console.log(opt.node);   // Path to node executable
console.log(opt.script); // Path to the current script
console.log(opt.opt);    // Command line options as a hash
console.log(opt.argv);   // Remaining non-option arguments
```

Example
-------
`node somescript.js foo -a=1 -b --c="hello world" bar ----d`

```js
// Result
opt.node   == "/path/to/node[.exe]"
opt.script == "/path/to/somescript.js"
opt.opt    == { a: 1, b: true, c: "hello world", d: true }
opt.argv   == ["foo", "bar"]
```

Full-featured test suite
------------------------
```js
#!/usr/bin/env node
console.log(require("./opt.js")());
```

License
-------
NASA Open Source Agreement v1.3
