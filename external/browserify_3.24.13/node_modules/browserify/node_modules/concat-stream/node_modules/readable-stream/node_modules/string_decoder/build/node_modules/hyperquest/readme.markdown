# hyperquest

treat http requests as a streaming transport

[![build status](https://secure.travis-ci.org/substack/hyperquest.png)](http://travis-ci.org/substack/hyperquest)

The hyperquest api is a subset of [request](https://github.com/mikeal/request).

This module works in the browser with [browserify](http://browserify.org).

# rant

![animated gif rant](http://substack.net/images/substack.gif)

This module disables a lot of infuriating things about core http that WILL cause
bugs in your application if you think of http as just another kind of stream:

* http requests have a default idle timeout of 2 minutes. This is terrible if
you just want to pipe together a bunch of persistent backend processes over
http.

* There is a default connection pool of 5 requests. If you have 5 or more extant
http requests, any additional requests will HANG for NO GOOD REASON.

hyperquest turns these annoyances off so you can just pretend that core http is
just a fancier version of tcp and not the horrible monstrosity that it actually
is.

I have it on good authority that these annoyances will be fixed in node 0.12.

# example

# simple streaming GET

``` js
var hyperquest = require('hyperquest');
hyperquest('http://localhost:8000').pipe(process.stdout);
```

```
$ node example/req.js
beep boop
```

# pooling is evil

Now to drive the point home about pooling being evil and almost always never
what you want ever.

[request](https://github.com/mikeal/request)
has its own forever agent thing that works pretty much the same as node core
http.request: the wrong, horrible, broken way.

For instance, the following request code takes 12+ seconds to finish:

``` js
var http = require('http');
var request = require('request');

var server = http.createServer(function (req, res) {
    res.write(req.url.slice(1) + '\n');
    setTimeout(res.end.bind(res), 3000);
});

server.listen(5000, function () {
    var pending = 20;
    for (var i = 0; i < 20; i++) {
        var r = request('http://localhost:5000/' + i);
        r.pipe(process.stdout, { end: false });
        r.on('end', function () {
            if (--pending === 0) server.close();
        });
    }
});

process.stdout.setMaxListeners(0); // turn off annoying warnings
```

```
substack : example $ time node many_request.js 
0
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19

real    0m12.423s
user    0m0.424s
sys 0m0.048s
```

Surprising? YES. This is pretty much never what you want, particularly if you
have a lot of streaming http API endpoints. Your code will just *HANG* once the
connection pool fills up and it won't start working again until some connections
die for whatever reason. I have encountered this so many times in production
instances and it is SO hard to track down reliably.

Compare to using hyperquest, which is exactly the same code but it takes 3
seconds instead of 12 to finish because it's not completely self-crippled like
request and core http.request.

``` js
var http = require('http');
var hyperquest = require('hyperquest');

var server = http.createServer(function (req, res) {
    res.write(req.url.slice(1) + '\n');
    setTimeout(res.end.bind(res), 3000);
});

server.listen(5000, function () {
    var pending = 20;
    for (var i = 0; i < 20; i++) {
        var r = hyperquest('http://localhost:5000/' + i);
        r.pipe(process.stdout, { end: false });
        r.on('end', function () {
            if (--pending === 0) server.close();
        });
    }
});

process.stdout.setMaxListeners(0); // turn off annoying warnings
```
```
$ time node many_hyperquest.js 
0
1
2
3
4
5
6
8
9
7
10
11
12
13
14
15
16
17
18
19

real    0m3.284s
user    0m0.288s
sys 0m0.060s
```

So the other thing is, the justification I've heard supporting this horrible
limit-of-5 pooling behavior is "performance". The first example which has been
tuned for "performance" takes 12 seconds. The second example that removes these
"performance" enhancements takes 3. Some performance improvement INDEED!

# methods

``` js
var hyperquest = require('hyperquest');
```

## var req = hyperquest(uri, opts={}, cb)

Create an outgoing http request to `uri` or `opts.uri`.
You need not pass any arguments here since there are setter methods documented
below.

Return a readable or duplex stream depending on the `opts.method`.

Default option values:

* opts.method - `"GET"`
* opts.headers - `{}`
* opts.auth - undefined, but is set automatically when the `uri` has an auth
string in it such as `"http://user:passwd@host"`. `opts.auth` is of the form
`"user:pass"`, just like `http.request()`.

In https mode, you can specify options to the underlying `tls.connect()` call:

* opts.pfx
* opts.key
* opts.cert
* opts.ca
* opts.ciphers
* opts.rejectUnauthorized
* opts.secureProtocol

The request does not go through until the `nextTick` so you can set values
outside of the `opts` so long as they are called on the same tick.

Optionally you can pass a `cb(err, res)` to set up listeners for `'error'` and
`'response'` events in one place.

Note that the optional `cb` is NOT like
[request](https://github.com/mikeal/request)
in that hyperquest will not buffer content for you or decode to json or any such
magical thing.

## req.setHeader(key, value);

Set an outgoing header `key` to `value`.

## req.setLocation(uri);

Set the location if you didn't specify it in the `hyperquest()` call.

## var req = hyperquest.get(uri, opts, cb)

Return a readable stream from `hyperquest(..., { method: 'GET' })`.

## var req = hyperquest.put(uri, opts, cb)

Return a duplex stream from `hyperquest(..., { method: 'PUT' })`.

## var req = hyperquest.post(uri, opts, cb)

Return a duplex stream from `hyperquest(..., { method: 'POST' })`.

## var req = hyperquest.delete(uri, opts, cb)

Return a readable stream from `hyperquest(..., { method: 'DELETE' })`.

# events

## req.on('response', function (res) {})

The `'response'` event is forwarded from the underlying `http.request()`.

## req.on('error', function (res) {})

The `'error'` event is forwarded from the underlying `http.request()`.

# install

With [npm](https://npmjs.org) do:

```
npm install hyperquest
```

# license

MIT
