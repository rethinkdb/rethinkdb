# ruglify

[![Build Status](https://secure.travis-ci.org/ForbesLindesay/ruglify.png)](http://travis-ci.org/ForbesLindesay/ruglify)
[![Dependency Status](https://gemnasium.com/ForbesLindesay/ruglify.png)](https://gemnasium.com/ForbesLindesay/ruglify)

"Require" minified JavaScript as a string

## Installation

```
npm install ruglify
```

## Usage

```javascript
var ruglify = require('ruglify');
var minifiedJQuery = ruglify('./jquery.js');
console.log(minifiedJQuery);
```

Uses the same resolution algorithm as require for maximum simplicity.  Built on top of [rfile](https://github.com/ForbesLindesay/rfile).

## License

  MIT