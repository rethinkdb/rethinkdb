# JSONStream

streaming JSON.parse and stringify

<img src=https://secure.travis-ci.org/dominictarr/JSONStream.png?branch=master>

## example

```javascript

var request = require('request')
  , JSONStream = require('JSONStream')
  , es = require('event-stream')

var parser = JSONStream.parse(['rows', true])
  , req = request({url: 'http://isaacs.couchone.com/registry/_all_docs'})
  , logger = es.mapSync(function (data) {
      console.error(data)
      return data
    })
```

in node 0.4.x

``` javascript

req.pipe(parser)
parser.pipe(logger)

```

in node v0.5.x

``` javascript
req.pipe(parser).pipe(logger)

```

## JSONStream.parse(path)

usally, a json API will return a list of objects.

`path` should be an array of property names, `RegExp`s, booleans, and/or functions.
any object that matches the path will be emitted as 'data' (and `pipe`d down stream)

a 'root' event is emitted when all data has been received. The 'root' event passes the root object & the count of matched objects.

if `path` is empty or null, no 'data' events are emitted.

### example

query a couchdb view:

``` bash
curl -sS localhost:5984/tests/_all_docs&include_docs=true
```
you will get something like this:

``` js
{"total_rows":129,"offset":0,"rows":[
  { "id":"change1_0.6995461115147918"
  , "key":"change1_0.6995461115147918"
  , "value":{"rev":"1-e240bae28c7bb3667f02760f6398d508"}
  , "doc":{
      "_id":  "change1_0.6995461115147918"
    , "_rev": "1-e240bae28c7bb3667f02760f6398d508","hello":1}
  },
  { "id":"change2_0.6995461115147918"
  , "key":"change2_0.6995461115147918"
  , "value":{"rev":"1-13677d36b98c0c075145bb8975105153"}
  , "doc":{
      "_id":"change2_0.6995461115147918"
    , "_rev":"1-13677d36b98c0c075145bb8975105153"
    , "hello":2
    }
  },
]}

```

we are probably most interested in the `rows.*.docs`

create a `Stream` that parses the documents from the feed like this:

``` js
var stream = JSONStream.parse(['rows', true, 'doc']) //rows, ANYTHING, doc

stream.on('data', function(data) {
  console.log('received:', data);
});

stream.on('root', function(root, count) {
  if (!count) {
    console.log('no matches found:', root);
  }
});
```
awesome!

## JSONStream.stringify(open, sep, close)

Create a writable stream.

you may pass in custom `open`, `close`, and `seperator` strings.
But, by default, `JSONStream.stringify()` will create an array,
(with default options `open='[\n', sep='\n,\n', close='\n]\n'`)

If you call `JSONStream.stringify(false)`
the elements will only be seperated by a newline.

If you only write one item this will be valid JSON.

If you write many items,
you can use a `RegExp` to split it into valid chunks.

## JSONStream.stringifyObject(open, sep, close)

Very much like `JSONStream.stringify`,
but creates a writable stream for objects instead of arrays.

Accordingly, `open='{\n', sep='\n,\n', close='\n}\n'`.

When you `.write()` to the stream you must supply an array with `[ key, data ]`
as the first argument.

## numbers

There are occasional problems parsing and unparsing very precise numbers.

I have opened an issue here:

https://github.com/creationix/jsonparse/issues/2

+1

## Acknowlegements

this module depends on https://github.com/creationix/jsonparse
by Tim Caswell
and also thanks to Florent Jaby for teaching me about parsing with:
https://github.com/Floby/node-json-streams

## license

MIT / APACHE2
