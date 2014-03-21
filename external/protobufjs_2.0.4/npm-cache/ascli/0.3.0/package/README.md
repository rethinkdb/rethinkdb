![ascli](https://raw.github.com/dcodeIO/ascli/master/ascli.png)
=====
**Why?** Some of us are not only programmers but also part-time artist. So am I. This is good. However, to limit myself
a bit to a straight look of my CLI apps, I've created ascli based on the thought of not making things too fancy but
still looking good. So, basically, this package is meant to be used by me but if you like my interpretation of
unobtrusiveness and ease-of-use ... You are welcome!

<p align="center">
    <img src="https://raw.github.com/dcodeIO/ascli/master/example.png" alt="example" />
</p>

Installation
------------
`npm install ascli`

Usage
-----
```js
var ascli = require("ascli").app("myApp");
ascli.banner(ascli.appName.green.bold, "v1.0.0 by Foo Bar <foobar@example.com>");
console.log("Hello!");
// If it worked:
ascli.ok("It worked!");
// If it didn't:
ascli.fail("Nope, sorry.", /* exit code */ 1);
```

#### Using another alphabet
By default ascli uses a modified version of the **straight** ASCII alphabet. If you don't like it, you are free to
replace it:

```js
ascli.use("/path/to/my/alphabet.json");
// or
var myAlphabet = { ... };
ascli.use(myAlphabet);
```

See the `alphabet/` directory for an example.

#### Using colors
ascli automatically looks up and translates ANSI terminal colors applied to the title string. For that it depends on
[colour.js](https://github.com/dcodeIO/colour.js) which is also exposed as a property of the ascli namespace:
`ascli.colour` / `ascli.colors`. Also means: You don't need another ANSI terminal colors dependency.

#### Indentation
ascli automatically indents all console output by one space just because it looks better with the banner.

Parsing command line arguments
------------------------------
[opt.js](https://github.com/dcodeIO/opt.js) will be pre-run on the `ascli` namespace and also exposed as `ascli.optjs()`.
```js
ascli.node   // Node executable
ascli.script // Executed script
ascli.opt    // Options as a hash
ascli.argv   // Remaining non-option arguments
```

License
-------
Apache License, Version 2.0
