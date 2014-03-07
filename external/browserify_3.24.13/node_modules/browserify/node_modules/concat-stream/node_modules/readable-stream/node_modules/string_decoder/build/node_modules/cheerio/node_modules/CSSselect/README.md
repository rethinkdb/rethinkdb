#CSSselect [![Build Status](https://secure.travis-ci.org/fb55/CSSselect.png?branch=master)](http://travis-ci.org/fb55/CSSselect)

a CSS selector compiler/engine

##What?

CSSselect turns CSS selectors into functions that tests if elements match them. When searching for elements, testing is executed "from the top", similar to how browsers execute CSS selectors.

In its default configuration, CSSselect queries the DOM structure of the [`domhandler`](https://github.com/fb55/domhandler) module.

##Why?

The common approach of executing CSS selectors (used eg. by [`Sizzle`](https://github.com/jquery/sizzle), [`nwmatcher`](https://github.com/dperini/nwmatcher/) and [`qwery`](https://github.com/ded/qwery)) is to execute every component of the selector in order, from left to right. The selector `a b` for example will first look for `a` elements, then search these for `b` elements.

While this works, it has some downsides: Children of `a`s will be checked multiple times, first, to check if they are also `a`s, then, for every superior `a` once, if they are `b`s. Using [Big O notation](http://en.wikipedia.org/wiki/Big_O_notation), that would be `O(n^2)`.

The far more efficient approach is to first look for `b` elements, then check if they have superior `a` elements: Using big O notation again, that would be `O(n)`.

And that's exactly what CSSselect does.

##How?

By stacking functions!

_//TODO: Better explanation. For now, if you're interested, have a look at the source code._

##API

```js
var CSSselect = require("CSSselect");
```

####`CSSselect(query, elems)`

- `query` can be either a function or a string. If it's a string, the string is compiled as a CSS selector.
- `elems` can be either an array of elements, or a single element. If it is an element, its children will be used (so we're working with an array again).

Queries `elems`, returns an array containing all matches.

Aliases: `CSSselect.selectAll(query, elems)`, `CSSselect.iterate(query, elems)`.

####`CSSselect.compile(query)`

Compiles the query, returns the function.

####`CSSselect.is(elem, query)`

Tests whether or not an element is matched by `query`. `query` can be either a CSS selector or a function.

####`CSSselect.selectOne(query, elems)`

Arguments are the same as for `CSSselect(query, elems)`. Only returns the first match, or `null` if there was no match.

---

License: BSD-like
