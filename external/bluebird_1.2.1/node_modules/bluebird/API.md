#API Reference

- [Core](#core)
    - [`new Promise(Function<Function resolve, Function reject> resolver)`](#new-promisefunctionfunction-resolve-function-reject-resolver---promise)
    - [`.then([Function fulfilledHandler] [, Function rejectedHandler ] [, Function progressHandler ])`](#thenfunction-fulfilledhandler--function-rejectedhandler---function-progresshandler----promise)
    - [`.catch(Function handler)`](#catchfunction-handler---promise)
    - [`.catch([Function ErrorClass|Function predicate...], Function handler)`](#catchfunction-errorclassfunction-predicate-function-handler---promise)
    - [`.error( [rejectedHandler] )`](#error-rejectedhandler----promise)
    - [`.finally(Function handler)`](#finallyfunction-handler---promise)
    - [`.tap(Function handler)`](#tapfunction-handler---promise)
    - [`.bind(dynamic thisArg)`](#binddynamic-thisarg---promise)
    - [`.done([Function fulfilledHandler] [, Function rejectedHandler ] [, Function progressHandler ])`](#donefunction-fulfilledhandler--function-rejectedhandler---function-progresshandler----promise)
    - [`Promise.try(Function fn [, Array<dynamic>|dynamic arguments] [, dynamic ctx] )`](#promisetryfunction-fn--arraydynamicdynamic-arguments--dynamic-ctx----promise)
    - [`Promise.method(Function fn)`](#promisemethodfunction-fn---function)
    - [`Promise.resolve(dynamic value)`](#promiseresolvedynamic-value---promise)
    - [`Promise.reject(dynamic reason)`](#promiserejectdynamic-reason---promise)
    - [`Promise.defer()`](#promisedefer---promiseresolver)
    - [`Promise.cast(dynamic value)`](#promisecastdynamic-value---promise)
    - [`Promise.bind(dynamic thisArg)`](#promisebinddynamic-thisarg---promise)
    - [`Promise.is(dynamic value)`](#promiseisdynamic-value---boolean)
    - [`Promise.longStackTraces()`](#promiselongstacktraces---void)
- [Progression](#progression)
    - [`.progressed(Function handler)`](#progressedfunction-handler---promise)
- [Promise resolution](#promise-resolution)
    - [`.resolve(dynamic value)`](#resolvedynamic-value---undefined)
    - [`.reject(dynamic reason)`](#rejectdynamic-reason---undefined)
    - [`.progress(dynamic value)`](#progressdynamic-value---undefined)
    - [`.callback`](#callback---function)
- [Timers](#timers)
    - [`.delay(int ms)`](#delayint-ms---promise)
    - [`.timeout(int ms [, String message])`](#timeoutint-ms--string-message---promise)
    - [`Promise.delay([dynamic value], int ms)`](#promisedelaydynamic-value-int-ms---promise)
- [Promisification](#promisification)
    - [`Promise.promisify(Function nodeFunction [, dynamic receiver])`](#promisepromisifyfunction-nodefunction--dynamic-receiver---function)
    - [`Promise.promisify(Object target)`](#promisepromisifyobject-target---object)
    - [`Promise.promisifyAll(Object target)`](#promisepromisifyallobject-target---object)
    - [`.nodeify([Function callback])`](#nodeifyfunction-callback---promise)
- [Cancellation](#cancellation)
    - [`.cancellable()`](#cancellable---promise)
    - [`.cancel()`](#cancel---promise)
    - [`.fork([Function fulfilledHandler] [, Function rejectedHandler ] [, Function progressHandler ])`](#forkfunction-fulfilledhandler--function-rejectedhandler---function-progresshandler----promise)
    - [`.uncancellable()`](#uncancellable---promise)
    - [`.isCancellable()`](#iscancellable---boolean)
- [Synchronous inspection](#synchronous-inspection)
    - [`.isFulfilled()`](#isfulfilled---boolean)
    - [`.isRejected()`](#isrejected---boolean)
    - [`.isPending()`](#isdefer---boolean)
    - [`.isResolved()`](#isresolved---boolean)
    - [`.value()`](#value---dynamic)
    - [`.reason()`](#reason---dynamic)
    - [`.inspect()`](#inspect---promiseinspection)
- [Generators](#generators)
    - [`Promise.coroutine(GeneratorFunction generatorFunction)`](#promisecoroutinegeneratorfunction-generatorfunction---function)
    - [`Promise.coroutine.addYieldHandler(function handler)`](#promisecoroutineaddyieldhandlerfunction-handler---void)
- [Utility](#utility)
    - [`.call(String propertyName [, dynamic arg...])`](#callstring-propertyname--dynamic-arg---promise)
    - [`.get(String propertyName)`](#getstring-propertyname---promise)
    - [`.return(dynamic value)`](#returndynamic-value---promise)
    - [`.throw(dynamic reason)`](#throwdynamic-reason---promise)
    - [`.toString()`](#tostring---string)
    - [`.toJSON()`](#tojson---object)
    - [`Promise.noConflict()`](#promisenoconflict---object)
    - [`Promise.onPossiblyUnhandledRejection(Function handler)`](#promiseonpossiblyunhandledrejectionfunction-handler---undefined)
    - [`Promise.onUnhandledRejectionHandled(Function handler)`](#promiseonunhandledrejectionhandledfunction-handler---undefined)
- [Collections](#collections)
    - [`.all()`](#all---promise)
    - [`.props()`](#props---promise)
    - [`.settle()`](#settle---promise)
    - [`.any()`](#any---promise)
    - [`.race()`](#race---promise)
    - [`.some(int count)`](#someint-count---promise)
    - [`.spread([Function fulfilledHandler] [, Function rejectedHandler ])`](#spreadfunction-fulfilledhandler--function-rejectedhandler----promise)
    - [`.map(Function mapper)`](#mapfunction-mapper---promise)
    - [`.reduce(Function reducer [, dynamic initialValue])`](#reducefunction-reducer--dynamic-initialvalue---promise)
    - [`.filter(Function filterer)`](#filterfunction-filterer---promise)
    - [`Promise.all(Array<dynamic>|Promise values)`](#promiseallarraydynamicpromise-values---promise)
    - [`Promise.props(Object|Promise object)`](#promisepropsobjectpromise-object---promise)
    - [`Promise.settle(Array<dynamic>|Promise values)`](#promisesettlearraydynamicpromise-values---promise)
    - [`Promise.any(Array<dynamic>|Promise values)`](#promiseanyarraydynamicpromise-values---promise)
    - [`Promise.race(Array|Promise promises)`](#promiseracearraypromise-promises---promise)
    - [`Promise.some(Array<dynamic>|Promise values, int count)`](#promisesomearraydynamicpromise-values-int-count---promise)
    - [`Promise.join([dynamic value...])`](#promisejoindynamic-value---promise)
    - [`Promise.map(Array<dynamic>|Promise values, Function mapper)`](#promisemaparraydynamicpromise-values-function-mapper---promise)
    - [`Promise.reduce(Array<dynamic>|Promise values, Function reducer [, dynamic initialValue])`](#promisereducearraydynamicpromise-values-function-reducer--dynamic-initialvalue---promise)
    - [`Promise.filter(Array<dynamic>|Promise values, Function filterer)`](#promisefilterarraydynamicpromise-values-function-filterer---promise)

##Core

Core methods of `Promise` instances and core static methods of the Promise class.

#####`new Promise(Function<Function resolve, Function reject> resolver)` -> `Promise`

Create a new promise. The passed in function will receive functions `resolve` and `reject` as its arguments which can be called to seal the fate of the created promise.

Example:

```js
function ajaxGetAsync(url) {
    return new Promise(function (resolve, reject) {
        var xhr = new XMLHttpRequest;
        xhr.addEventListener("error", reject);
        xhr.addEventListener("load", resolve);
        xhr.open("GET", url);
        xhr.send(null);
    });
}
```

If you pass a promise object to the `resolve` function, the created promise will follow the state of that promise.

<hr>

To make sure a function that returns a promise is following the implicit but critically important contract of promises, you can start a function with `new Promise` if you cannot start a chain immediately:

```js
function getConnection(urlString) {
    return new Promise(function(resolve) {
        //Without new Promise, this throwing will throw an actual exception
        var params = parse(urlString);
        resolve(getAdapater(params).getConnection());
    });
}
```

The above ensures `getConnection()` fulfills the contract of a promise-returning function of never throwing a synchronous exception. Also see [`Promise.try`](#promisetryfunction-fn--arraydynamicdynamic-arguments--dynamic-ctx----promise) and [`Promise.method`](#promisemethodfunction-fn---function)

<hr>

#####`.then([Function fulfilledHandler] [, Function rejectedHandler ] [, Function progressHandler ])` -> `Promise`

[Promises/A+ `.then()`](http://promises-aplus.github.io/promises-spec/) with progress handler. Returns a new promise chained from this promise. The new promise will be rejected or resolved dedefer on the passed `fulfilledHandler`, `rejectedHandler` and the state of this promise.

Example:

```js
promptAsync("Which url to visit?").then(function(url){
    return ajaxGetAsync(url);
}).then(function(contents){
    alertAsync("The contents were: " + contents);
}).catch(function(e){
    alertAsync("Exception " + e);
});
```

<hr>

#####`.catch(Function handler)` -> `Promise`

This is a catch-all exception handler, shortcut for calling `.then(null, handler)` on this promise. Any exception happening in a `.then`-chain will propagate to nearest `.catch` handler.

*For compatibility with earlier ECMAScript version, an alias `.caught()` is provided for `.catch()`.*

<hr>

#####`.catch([Function ErrorClass|Function predicate...], Function handler)` -> `Promise`

This extends `.catch` to work more like catch-clauses in languages like Java or C#. Instead of manually checking `instanceof` or `.name === "SomeError"`, you may specify a number of error constructors which are eligible for this catch handler. The catch handler that is first met that has eligible constructors specified, is the one that will be called.

Example:

```js
somePromise.then(function(){
    return a.b.c.d();
}).catch(TypeError, function(e){
    //If a is defined, will end up here because
    //it is a type error to reference property of undefined
}).catch(ReferenceError, function(e){
    //Will end up here if a wasn't defined at all
}).catch(function(e){
    //Generic catch-the rest, error wasn't TypeError nor
    //ReferenceError
});
 ```

You may also add multiple filters for a catch handler:

```js
somePromise.then(function(){
    return a.b.c.d();
}).catch(TypeError, ReferenceError, function(e){
    //Will end up here on programmer error
}).catch(NetworkError, TimeoutError, function(e){
    //Will end up here on expected everyday network errors
}).catch(function(e){
    //Catch any unexpected errors
});
```

For a parameter to be considered a type of error that you want to filter, you need the constructor to have its `.prototype` property be `instanceof Error`.

Such a constructor can be minimally created like so:

```js
function MyCustomError() {}
MyCustomError.prototype = Object.create(Error.prototype);
```

Using it:

```js
Promise.resolve().then(function(){
    throw new MyCustomError();
}).catch(MyCustomError, function(e){
    //will end up here now
});
```

However if you  want stack traces and cleaner string output, then you should do:

*in Node.js and other V8 environments, with support for `Error.captureStackTrace`*

```js
function MyCustomError(message) {
    this.message = message;
    this.name = "MyCustomError";
    Error.captureStackTrace(this, MyCustomError);
}
MyCustomError.prototype = Object.create(Error.prototype);
MyCustomError.prototype.constructor = MyCustomError;
```

Using CoffeeScript's `class` for the same:

```coffee
class MyCustomError extends Error
  constructor: (@message) ->
    @name = "MyCustomError"
    Error.captureStackTrace(this, MyCustomError)
```

This method also supports predicate-based filters. If you pass a
predicate function instead of an error constructor, the predicate will receive
the error as an argument. The return result of the predicate will be used
determine whether the error handler should be called.

Predicates should allow for very fine grained control over caught errors:
pattern matching, error-type sets with set operations and many other techniques
can be implemented on top of them.

Example of using a predicate-based filter:

```js
var Promise = require("bluebird");
var request = Promise.promisify(require("request"));

function clientError(e) {
    return e.code >= 400 && e.code < 500;
}

request("http://www.google.com").then(function(contents){
    console.log(contents);
}).catch(clientError, function(e){
   //A client error like 400 Bad Request happened
});
```

*For compatibility with earlier ECMAScript version, an alias `.caught()` is provided for `.catch()`.*

<hr>

#####`.error( [rejectedHandler] )` -> `Promise`

Like `.catch` but instead of catching all types of exceptions, it only catches those that don't originate from thrown errors but rather from explicit rejections.

*Note, "errors" mean errors, as in objects that are `instanceof Error` - not strings, numbers and so on. See [a string is not an error](http://www.devthought.com/2011/12/22/a-string-is-not-an-error/).*

For example, if a promisified function errbacks the node-style callback with an error, that could be caught with `.error()`. However if the node-style callback **throws** an error, only `.catch` would catch that.

In the following example you might want to handle just the `SyntaxError` from JSON.parse and Filesystem errors from `fs` but let programmer errors bubble as unhandled rejections:

```js
var fs = Promise.promisifyAll(require("fs"));

fs.readFileAsync("myfile.json").then(JSON.parse).then(function (json) {
    console.log("Successful json")
}).catch(SyntaxError, function (e) {
    console.error("file contains invalid json");
}).error(function (e) {
    console.error("unable to read file, because: ", e.message);
});
```

Now, because there is no catch-all handler, if you typed `console.lag` (causes an error you don't expect), you will see:

```
Possibly unhandled TypeError: Object #<Console> has no method 'lag'
    at application.js:8:13
From previous event:
    at Object.<anonymous> (application.js:7:4)
    at Module._compile (module.js:449:26)
    at Object.Module._extensions..js (module.js:467:10)
    at Module.load (module.js:349:32)
    at Function.Module._load (module.js:305:12)
    at Function.Module.runMain (module.js:490:10)
    at startup (node.js:121:16)
    at node.js:761:3
```

*( If you don't get the above - you need to enable [long stack traces](#promiselongstacktraces---void) )*

And if the file contains invalid JSON:

```
file contains invalid json
```

And if the `fs` module causes an error like file not found:

```
unable to read file, because:  ENOENT, open 'not_there.txt'
```

<hr>

#####`.finally(Function handler)` -> `Promise`

Pass a handler that will be called regardless of this promise's fate. Returns a new promise chained from this promise. There are special semantics for `.finally()` in that the final value cannot be modified from the handler.

Consider the example:

```js
function anyway() {
    $("#ajax-loader-animation").hide();
}

function ajaxGetAsync(url) {
    return new Promise(function (resolve, reject) {
        var xhr = new XMLHttpRequest;
        xhr.addEventListener("error", reject);
        xhr.addEventListener("load", resolve);
        xhr.open("GET", url);
        xhr.send(null);
    }).then(anyway, anyway);
}
```

This example doesn't work as intended because the `then` handler actually swallows the exception and returns `undefined` for any further chainers.

The situation can be fixed with `.finally`:

```js
function ajaxGetAsync(url) {
    return new Promise(function (resolve, reject) {
        var xhr = new XMLHttpRequest;
        xhr.addEventListener("error", reject);
        xhr.addEventListener("load", resolve);
        xhr.open("GET", url);
        xhr.send(null);
    }).finally(function(){
        $("#ajax-loader-animation").hide();
    });
}
```

Now the animation is hidden but an exception or the actual return value will automatically skip the finally and propagate to further chainers. This is more in line with the synchronous `finally` keyword.

The `.finally` works like [Q's finally method](https://github.com/kriskowal/q/wiki/API-Reference#promisefinallycallback).

*For compatibility with earlier ECMAScript version, an alias `.lastly()` is provided for `.finally()`.*

<hr>

#####`.tap(Function handler)` -> `Promise`

Like `.finally` that is not called for rejections.

```js
getUser().tap(function(user) {
    //Like in finally, if you return a promise from the handler
    //the promise is awaited for before passing the original value through
    return recordStatsAsync();
}).then(function(user) {
    //user is the user from getUser(), not recordStatsAsync()
});
```

<hr>

#####`.bind(dynamic thisArg)` -> `Promise`

Create a promise that follows this promise, but is bound to the given `thisArg` value. A bound promise will call its handlers with the bound value set to `this`. Additionally promises derived from a bound promise will also be bound promises with the same `thisArg` binding as the original promise.

<hr>

Without arrow functions that provide lexical `this`, the correspondence between async and sync code breaks down when writing object-oriented code. `.bind()` alleviates this.

Consider:

```js
MyClass.prototype.method = function() {
    try {
        var contents = fs.readFileSync(this.file);
        var url = urlParse(contents);
        var result = this.httpGetSync(url);
        var refined = this.refine(result);
        return this.writeRefinedSync(refined);
    }
    catch (e) {
        this.error(e.stack);
    }
};
```

The above has a direct translation:

```js
MyClass.prototype.method = function() {
    return fs.readFileAsync(this.file).bind(this)
    .then(function(contents) {
        var url = urlParse(contents);
        return this.httpGetAsync(url);
    }).then(function(result){
        var refined = this.refine(result);
        return this.writeRefinedAsync(refined);
    }).catch(function(e){
        this.error(e.stack);
    });
};
```

`.bind()` is the most efficient way of utilizing `this` with promises. The handler functions in the above code are not closures and can therefore even be hoisted out if needed. There is literally no overhead when propagating the bound value from one promise to another.

<hr>

`.bind()` also has a useful side purpose - promise handlers don't need to share a function to use shared state:

```js
somethingAsync().bind({})
.then(function (aValue, bValue) {
    this.aValue = aValue;
    this.bValue = bValue;
    return somethingElseAsync(aValue, bValue);
})
.then(function (cValue) {
    return this.aValue + this.bValue + cValue;
});
```

The above without `.bind()` could be achieved with:

```js
var scope = {};
somethingAsync()
.then(function (aValue, bValue) {
    scope.aValue = aValue;
    scope.bValue = bValue;
    return somethingElseAsync(aValue, bValue);
})
.then(function (cValue) {
    return scope.aValue + scope.bValue + cValue;
});
```

However, there are many differences when you look closer:

- Requires a statement so cannot be used in an expression context
- If not there already, an additional wrapper function is required to avoid leaking or sharing `scope`
- The handler functions are now closures, thus less efficient and not reusable

<hr>

Note that bind is only propagated with promise transformation. If you create new promise chains inside a handler, those chains are not bound to the "upper" `this`:

```js
something().bind(var1).then(function(){
    //`this` is var1 here
    return Promise.all(getStuff()).then(function(results){
        //`this` is undefined here
        //refine results here etc
    });
}).then(function(){
    //`this` is var1 here
});
```

However, if you are utilizing the full bluebird API offering, you will *almost never* need to resort to nesting promises in the first place. The above should be written more like:

```js
something().bind(var1).then(function() {
    //`this` is var1 here
    return getStuff();
}).map(function(result){
    //`this` is var1 here
    //refine result here
}).then(function(){
    //`this` is var1 here
});
```

Also see [this Stackoverflow answer](http://stackoverflow.com/a/19467053/995876) on a good example on how utilizing the collection instance methods like [`.map()`](#mapfunction-mapper---promise) can clean up code.

<hr>

If you don't want to return a bound promise to the consumers of a promise, you can rebind the chain at the end:

```js
MyClass.prototype.method = function() {
    return fs.readFileAsync(this.file).bind(this)
    .then(function(contents) {
        var url = urlParse(contents);
        return this.httpGetAsync(url);
    }).then(function(result){
        var refined = this.refine(result);
        return this.writeRefinedAsync(refined);
    }).catch(function(e){
        this.error(e.stack);
    }).bind(); //The `thisArg` is implicitly undefined - I.E. the default promise `this` value
};
```

Rebinding can also be abused to do something gratuitous like this:

```js
Promise.resolve("my-element")
    .bind(document)
    .then(document.getElementById)
    .bind(console)
    .then(console.log);
```

The above does `console.log(document.getElementById("my-element"));`. The `.bind()`s are necessary because in browser neither of the methods can be called as a stand-alone function.

<hr>

#####`.done([Function fulfilledHandler] [, Function rejectedHandler ] [, Function progressHandler ])` -> `void`

Like `.then()`, but any unhandled rejection that ends up here will be thrown as an error.

<hr>

#####`Promise.try(Function fn [, Array<dynamic>|dynamic arguments] [, dynamic ctx] )` -> `Promise`

Start the chain of promises with `Promise.try`. Any synchronous exceptions will be turned into rejections on the returned promise.

```js
function getUserById(id) {
    return Promise.try(function(){
        if (typeof id !== "number") {
            throw new Error("id must be a number");
        }
        return db.getUserById(id);
    });
}
```

Now if someone uses this function, they will catch all errors in their Promise `.catch` handlers instead of having to handle both synchronous and asynchronous exception flows.

Note about second argument: if it's specifically a true array, its values become respective arguments for the function call. Otherwise it is passed as is as the first argument for the function call.

*For compatibility with earlier ECMAScript version, an alias `Promise.attempt()` is provided for `Promise.try()`.*

<hr>

#####`Promise.method(Function fn)` -> `Function`

Returns a new function that wraps the given function `fn`. The new function will always return a promise that is fulfilled with the original functions return values or rejected with thrown exceptions from the original function.

This method is convenient when a function can sometimes return synchronously or throw synchronously.

Example without using `Promise.method`:

```js
MyClass.prototype.method = function(input) {
    if (!this.isValid(input)) {
        return Promise.reject(new TypeError("input is not valid"));
    }

    if (this.cache(input)) {
        return Promise.resolve(this.someCachedValue);
    }

    return db.queryAsync(input).bind(this).then(function(value) {
        this.someCachedValue = value;
        return value;
    });
};
```

Using the same function `Promise.method`, there is no need to manually wrap direct return or throw values into a promise:

```js
MyClass.prototype.method = Promise.method(function(input) {
    if (!this.isValid(input)) {
        throw new TypeError("input is not valid");
    }

    if (this.cachedFor(input)) {
        return this.someCachedValue;
    }

    return db.queryAsync(input).bind(this).then(function(value) {
        this.someCachedValue = value;
        return value;
    });
});
```

<hr>

#####`Promise.resolve(dynamic value)` -> `Promise`

Create a promise that is resolved with the given `value`. If `value` is a thenable or promise, the returned promise will assume its state.

<hr>

#####`Promise.reject(dynamic reason)` -> `Promise`

Create a promise that is rejected with the given `reason`.

<hr>

#####`Promise.defer()` -> `PromiseResolver`

Create a promise with undecided fate and return a `PromiseResolver` to control it. See [Promise resolution](#promise-resolution).

The use of `Promise.defer` is discouraged - it is much more awkward and error-prone than using `new Promise`.

<hr>

#####`Promise.cast(dynamic value)` -> `Promise`

Cast the given `value` to a trusted promise. If `value` is already a trusted `Promise`, it is returned as is. If `value` is not a thenable, a fulfilled Promise is returned with `value` as its fulfillment value. If `value` is a thenable (Promise-like object, like those returned by jQuery's `$.ajax`), returns a trusted Promise that assimilates the state of the thenable.

Example: (`$` is jQuery)

```js
Promise.cast($.get("http://www.google.com")).then(function(){
    //Returning a thenable from a handler is automatically
    //cast to a trusted Promise as per Promises/A+ specification
    return $.post("http://www.yahoo.com");
}).then(function(){

}).catch(function(e){
    //jQuery doesn't throw real errors so use catch-all
    console.log(e.statusText);
});
```

<hr>

#####`Promise.bind(dynamic thisArg)` -> `Promise`

Sugar for `Promise.resolve(undefined).bind(thisArg);`. See [`.bind()`](#binddynamic-thisarg---promise).

<hr>

#####`Promise.is(dynamic value)` -> `boolean`

See if `value` is a trusted Promise.

```js
Promise.is($.get("http://www.google.com")); //false , thenable returned from $.get is not a `Promise` instance
Promise.is(Promise.cast($.get("http://www.google.com"))) //true, `.cast` cast the thenable into a `Promise`
```

Trusted Promises are promises originating in the currently used copy of Bluebird. Promises originating in other libraries are called thenables and are _not_ trusted promises. This method is used for checking if a passed value is of the same type as `Promise` itself creates.

<hr>

#####`Promise.longStackTraces()` -> `void`

Call this right after the library is loaded to enabled long stack traces. Long stack traces cannot be disabled after being enabled, and cannot be enabled after promises have alread been created. Long stack traces imply a substantial performance penalty, around 4-5x for throughput and 0.5x for latency.

Long stack traces are enabled by default in the debug build.

To enable them in all instances of bluebird in node.js, use the environment variable `BLUEBIRD_DEBUG`:

```
BLUEBIRD_DEBUG=1 node server.js
```

You should enabled long stack traces if you want better debugging experience. For example:

```js
Promise.longStackTraces();
Promise.resolve().then(function outer() {
    return Promise.resolve().then(function inner() {
        return Promise.resolve().then(function evenMoreInner() {
            a.b.c.d()
        }).catch(function catcher(e){
            console.error(e.stack);
        });
    });
});
```

Gives

    ReferenceError: a is not defined
        at evenMoreInner (<anonymous>:6:13)
    From previous event:
        at inner (<anonymous>:5:24)
    From previous event:
        at outer (<anonymous>:4:20)
    From previous event:
        at <anonymous>:3:9
        at Object.InjectedScript._evaluateOn (<anonymous>:581:39)
        at Object.InjectedScript._evaluateAndWrap (<anonymous>:540:52)
        at Object.InjectedScript.evaluate (<anonymous>:459:21)

While with long stack traces disabled, you would get:

    ReferenceError: a is not defined
        at evenMoreInner (<anonymous>:6:13)
        at tryCatch1 (<anonymous>:41:19)
        at Promise$_resolvePromise [as _resolvePromise] (<anonymous>:1739:13)
        at Promise$_resolveLast [as _resolveLast] (<anonymous>:1520:14)
        at Async$_consumeFunctionBuffer [as _consumeFunctionBuffer] (<anonymous>:560:33)
        at Async$consumeFunctionBuffer (<anonymous>:515:14)
        at MutationObserver.Promise$_Deferred (<anonymous>:433:17)

On client side, long stack traces currently only work in Firefox and Chrome.

<hr>

##Progression

#####`.progressed(Function handler)` -> `Promise`

Shorthand for `.then(null, null, handler);`. Attach a progress handler that will be called if this promise is progressed. Returns a new promise chained from this promise.

<hr>

##Promise resolution

A `PromiseResolver` can be used to control the fate of a promise. It is like "Deferred" known in jQuery. The `PromiseResolver` objects have a `.promise` property which returns a reference to the controlled promise that can be passed to clients. `.promise` of a `PromiseResolver` is not a getter function to match other implementations.

The methods of a `PromiseResolver` have no effect if the fate of the underlying promise is already decided (follow, reject, fulfill).

The use of `Promise.defer` and deferred objects is discouraged - it is much more awkward and error-prone than using `new Promise`.

<hr>

#####`.resolve(dynamic value)` -> `undefined`

Resolve the underlying promise with `value` as the resolution value. If `value` is a thenable or a promise, the underlying promise will assume its state.

<hr>

#####`.reject(dynamic reason)` -> `undefined`

Reject the underlying promise with `reason` as the rejection reason.

<hr>

#####`.progress(dynamic value)` -> `undefined`

Progress the underlying promise with `value` as the progression value.

Example

```js
function delay(ms) {
    var resolver = Promise.defer();
    var now = Date.now();
    setTimeout(function(){
        resolver.resolve(Date.now() - now);
    }, ms);
    return resolver.promise;
}

delay(500).then(function(ms){
    console.log(ms + " ms passed");
});
```

<hr>

#####`.callback` -> `Function`

Gives you a callback representation of the `PromiseResolver`. Note that this is not a method but a property. The callback accepts error object in first argument and success values on the 2nd parameter and the rest, I.E. node js conventions.

If the the callback is called with multiple success values, the resolver fullfills its promise with an array of the values.

```js
var fs = require("fs");
function readAbc() {
    var resolver = Promise.defer();
    fs.readFile("abc.txt", resolver.callback);
    return resolver.promise;
}

readAbc()
.then(function(abcContents) {
    console.log(abcContents);
})
.catch(function(e) {
    console.error(e);
});
```

This example is an alternative to automatic promisification of node functions.

*Performance tips*

The `callback` is actually an accessor property (except on legacy browsers where it's eager data property) - so save the result if you need to call it multiple times.

This is more efficient way of promisification than using `new Promise`.

<hr>

##Timers

Methods to delay and time promises out.

#####`.delay(int ms)` -> `Promise`

Same as calling [`Promise.delay(this, ms)`](#promisedelaydynamic-value-int-ms---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>

#####`.timeout(int ms [, String message])` -> `Promise`

Returns a promise that will be fulfilled with this promise's fulfillment value or rejection reason. However, if this promise is not fulfilled or rejected within `ms` milliseconds, the returned promise is rejected with a `Promise.TimeoutError` instance.

You may specify a custom error message with the `message` parameter.

The example function `fetchContent` tries to fetch the contents of a web page with a 50ms timeout and sleeping 100ms between each retry. If there is no response after 5 retries, then the returned promise is rejected with a `ServerError` (made up error type). Additionally the whole process can be cancelled from outside at any point.

```js
function fetchContent(retries) {
    if (!retries) retries = 0;
    var jqXHR = $.get("http://www.slowpage.com");
    //Cast the jQuery promise into a bluebird promise
    return Promise.cast(jqXHR)
        .cancellable()
        .timeout(50)
        .catch(Promise.TimeoutError, function() {
            if (retries < 5) {
                return Promise.delay(100).then(function(){
                    return fetchContent(retries+1);
                });
            }
            else {
                throw new ServerError("not responding after 5 retries");
            }
        })
        .catch(Promise.CancellationError, function(er) {
            jqXHR.abort();
            throw er; //Don't swallow it
        });
}
```

<hr>

#####`Promise.delay([dynamic value], int ms)` -> `Promise`

Returns a promise that will be fulfilled with `value` (or `undefined`) after given `ms` milliseconds. If `value` is a promise, the delay will start counting down when it is fulfilled and the returned promise will be fulfilled with the fulfillment value of the `value` promise.

```js
Promise.delay(500).then(function(){
    console.log("500 ms passed");
    return "Hello world";
}).delay(500).then(function(helloWorldString) {
    console.log(helloWorldString);
    console.log("another 500 ms passed") ;
});
```

<hr>

##Promisification

#####`Promise.promisify(Function nodeFunction [, dynamic receiver])` -> `Function`

Returns a function that will wrap the given `nodeFunction`. Instead of taking a callback, the returned function will return a promise whose fate is decided by the callback behavior of the given node function. The node function should conform to node.js convention of accepting a callback as last argument and calling that callback with error as the first argument and success value on the second argument.

If the `nodeFunction` calls its callback with multiple success values, the fulfillment value will be an array of them.

If you pass a `receiver`, the `nodeFunction` will be called as a method on the `receiver`.

Example of promisifying the asynchronous `readFile` of node.js `fs`-module:

```js
var readFile = Promise.promisify(require("fs").readFile);

readFile("myfile.js", "utf8").then(function(contents){
    return eval(contents);
}).then(function(result){
    console.log("The result of evaluating myfile.js", result);
}).catch(SyntaxError, function(e){
    console.log("File had syntax error", e);
//Catch any other error
}).catch(function(e){
    console.log("Error reading file", e);
});
```

Note that if the node function is a method of some object, you need to pass the object as the second argument like so:

```js
var redisGet = Promise.promisify(redisClient.get, redisClient);
redisGet.then(function(){
    //...
});
```

**Tip**

Use [`.spread`](#spreadfunction-fulfilledhandler--function-rejectedhandler----promise) with APIs that have multiple success values:

```js
var Promise = require("bluebird");
var request = Promise.promisify(require('request'));
request("http://www.google.com").spread(function(request, body) {
    console.log(body);
}).catch(function(err) {
    console.error(err);
});
```

The above uses [request](https://github.com/mikeal/request) library which has a callback signature of multiple success values.

<hr>

#####`Promise.promisify(Object target)` -> `Object`

This overload has been **deprecated**. The overload will continue working for now. The recommended method for promisifying multiple methods at once is [`Promise.promisifyAll(Object target)`](#promisepromisifyallobject-target---object)

<hr>

#####`Promise.promisifyAll(Object target)` -> `Object`

Promisifies the entire object by going through the object's properties and creating an async equivalent of each function on the object and its prototype chain. The promisified method name will be the original method name postfixed with `Async`. Returns the input object.

Note that the original methods on the object are not overwritten but new methods are created with the `Async`-postfix. For example, if you `promisifyAll()` the node.js `fs` object use `fs.statAsync()` to call the promisified `stat` method.

Example:

```js
Promise.promisifyAll(RedisClient.prototype);

//Later on, all redis client instances have promise returning functions:

redisClient.hexistsAsync("myhash", "field").then(function(v){

}).catch(function(e){

});
```

If you don't want to write on foreign prototypes, you can sub-class the target and promisify your subclass:

```js
function MyRedisClient() {
    RedisClient.apply(this, arguments);
}
MyRedisClient.prototype = Object.create(RedisClient.prototype);
MyRedisClient.prototype.constructor = MyRedisClient;
Promise.promisify(MyRedisClient.prototype);
```

The promisified methods will be written on the `MyRedisClient.prototype` instead. This specific example doesn't actually work with `node_redis` because the `createClient` factory is hardcoded to instantiate `RedisClient` from closure.


It also works on singletons or specific instances:

```js
var fs = Promise.promisifyAll(require("fs"));

fs.readFileAsync("myfile.js", "utf8").then(function(contents){
    console.log(contents);
}).catch(function(e){
    console.error(e.stack);
});
```

The entire prototype chain of the object is promisified on the object. Only enumerable are considered. If the object already has a promisified version of the method, it will be skipped. The target methods are assumed to conform to node.js callback convention of accepting a callback as last argument and calling that callback with error as the first argument and success value on the second argument. If the node method calls its callback with multiple success values, the fulfillment value will be an array of them.

If a method already has `"Async"` postfix, it will be duplicated. E.g. `getAsync`'s promisified name is `getAsyncAsync`.

<hr>

#####`.nodeify([Function callback])` -> `Promise`

Register a node-style callback on this promise. When this promise is is either fulfilled or rejected, the node callback will be called back with the node.js convention where error reason is the first argument and success value is the second argument. The error argument will be `null` in case of success.

Returns back this promise instead of creating a new one. If the `callback` argument is not a function, this method does not do anything.

This can be used to create APIs that both accept node-style callbacks and return promises:

```js
function getDataFor(input, callback) {
    return dataFromDataBase(input).nodeify(callback);
}
```

The above function can then make everyone happy.

Promises:

```js
getDataFor("me").then(function(dataForMe) {
    console.log(dataForMe);
});
```

Normal callbacks:

```js
getDataFor("me", function(err, dataForMe) {
    if( err ) {
        console.error( err );
    }
    console.log(dataForMe);
});
```

There is no effect on peformance if the user doesn't actually pass a node-style callback function.

<hr>

##Cancellation

By default, a promise is not cancellable. A promise can be marked as cancellable with `.cancellable()`. A cancellable promise can be cancelled if it's not resolved. Cancelling a promise propagates to the farthest cancellable ancestor of the target promise that is still pending, and rejects that promise with `CancellationError`. The rejection will then propagate back to the original promise and to its descendants. This roughly follows the semantics described [here](https://github.com/promises-aplus/cancellation-spec/issues/7).

Promises marked with `.cancellable()` return cancellable promises automatically.

If you are the resolver for a promise, you can react to a cancel in your promise by catching the `CancellationError`:

```js
function ajaxGetAsync(url) {
    var xhr = new XMLHttpRequest;
    return new Promise(function (resolve, reject) {
        xhr.addEventListener("error", reject);
        xhr.addEventListener("load", resolve);
        xhr.open("GET", url);
        xhr.send(null);
    }).cancellable().catch(Promise.CancellationError, function(e) {
        xhr.abort();
        throw e; //Don't swallow it
    });
}
```

<hr>

#####`.cancellable()` -> `Promise`

Marks this promise as cancellable. Promises by default are not cancellable after v0.11 and must be marked as such for [`.cancel()`](#cancel---promise) to have any effect. Marking a promise as cancellable is infectious and you don't need to remark any descendant promise.

If you have code written prior v0.11 using cancellation, add calls to `.cancellable()` at the starts of promise chains that need to support
cancellation in themselves or somewhere in their descendants.

<hr>

#####`.cancel()` -> `Promise`

Cancel this promise. The cancellation will propagate
to farthest cancellable ancestor promise which is still pending.

That ancestor will then be rejected with a `CancellationError` (get a reference from `Promise.CancellationError`)
object as the rejection reason.

In a promise rejection handler you may check for a cancellation
by seeing if the reason object has `.name === "Cancel"`.

Promises are by default not cancellable. Use [`.cancellable()`](#cancellable---promise) to mark a promise as cancellable.

<hr>

#####`.fork([Function fulfilledHandler] [, Function rejectedHandler ] [, Function progressHandler ])` -> `Promise`

Like `.then()`, but cancellation of the the returned promise
or any of its descendant will not propagate cancellation
to this promise or this promise's ancestors.

<hr>

#####`.uncancellable()` -> `Promise`

Create an uncancellable promise based on this promise.

<hr>

#####`.isCancellable()` -> `boolean`

See if this promise can be cancelled.

<hr>

##Synchronous inspection

Because `.then()` must give asynchronous guarantees, it cannot be used to inspect a given promise's state synchronously. The following code won't work:

```js
var wasFulfilled = false;
var wasRejected = false;
var resolutionValueOrRejectionReason = null;
somePromise.then(function(v){
    wasFulfilled = true;
    resolutionValueOrRejectionReason = v
}).catch(function(v){
    wasRejected = true;
    resolutionValueOrRejectionReason = v
});
//Using the variables won't work here because .then must be called asynchronously
```

Synchronous inspection API allows you to do this like so:

```js
var inspection = somePromise.inspect();

if(inspection.isFulfilled()){
    console.log("Was fulfilled with", inspection.value());
}
```

<hr>

#####`.isFulfilled()` -> `boolean`

See if this `promise` has been fulfilled.

<hr>

#####`.isRejected()` -> `boolean`

See if this `promise` has been rejected.

<hr>

#####`.isPending()` -> `boolean`

See if this `promise` is still defer.

<hr>

#####`.isResolved()` -> `boolean`

See if this `promise` is resolved -> either fulfilled or rejected.

<hr>

#####`.value()` -> `dynamic`

Get the fulfillment value of this promise. Throws an error if the promise isn't fulfilled.

<hr>

#####`.reason()` -> `dynamic`

Get the rejection reason of this promise. Throws an error if the promise isn't rejected.

<hr>

#####`.inspect()` -> `PromiseInspection`

Synchronously inspect the state of this `promise`. The `PromiseInspection` will represent the state of the promise as snapshotted at the time of calling `.inspect()`. It will have the following methods:

`.isFulfilled()` -> `boolean`

See if the underlying promise was fulfilled at the creation time of this inspection object.

`.isRejected()` -> `boolean`

See if the underlying promise was rejected at the creation time of this inspection object.

`.isPending()` -> `boolean`

See if the underlying promise was defer at the creation time of this inspection object.

`.value()` -> `dynamic`, throws `TypeError`

Get the fulfillment value of the underlying promise. Throws if the promise wasn't fulfilled at the creation time of this inspection object.

`.error()` -> `dynamic`, throws `TypeError`

Get the rejection reason for the underlying promise. Throws if the promise wasn't rejected at the creation time of this inspection object.

<hr>

##Generators

Using ECMAScript6 generators feature to implement C# 5.0 `async/await` like syntax.

#####`Promise.coroutine(GeneratorFunction generatorFunction)` -> `Function`

Returns a function that can use `yield` to run asynchronous code synchronously. This feature requires the support of generators which are drafted in the next version of the language. Node version greater than `0.11.2` is required and needs to be executed with the `--harmony-generators` (or `--harmony`) command-line switch.

This is the recommended, simplest and most performant way of using asynchronous generators with bluebird. It is even faster than typical promise code because the creation of new anonymous function identities at runtime can be completely avoided without obfuscating your code.

```js
var Promise = require("bluebird");

function delay(ms) {
    return new Promise(function(f){
        setTimeout(f, ms);
    });
}

function PingPong() {

}

PingPong.prototype.ping = Promise.coroutine(function* (val) {
    console.log("Ping?", val)
    yield delay(500)
    this.pong(val+1)
});

PingPong.prototype.pong = Promise.coroutine(function* (val) {
    console.log("Pong!", val)
    yield delay(500);
    this.ping(val+1)
});

var a = new PingPong();
a.ping(0);
```

Running the example with node version at least 0.11.2:

    $ node --harmony test.js
    Ping? 0
    Pong! 1
    Ping? 2
    Pong! 3
    Ping? 4
    Pong! 5
    Ping? 6
    Pong! 7
    Ping? 8
    ...

When called, the coroutine function will start an instance of the generator and returns a promise for its final value.

Doing `Promise.coroutine(function*(){})` is almost like using the C# `async` keyword to mark the function, with `yield` working as the `await` keyword. Promises are somewhat like `Task`s.

**Tip**

If you yield an array then its elements are implicitly waited for. You may add your own custom special treatments with [`Promise.coroutine.addYieldHandler`](#promisecoroutineaddyieldhandlerfunction-handler---void)

You can combine it with ES6 destructuring for some neat syntax:

```js
var getData = Promise.coroutine(function* (urlA, urlB) {
    [resultA, resultB] = yield [http.getAsync(urlA), http.getAsync(urlB)];
    //use resultA
    //use resultB
});
```

You might wonder why not just do this?

```js
var getData = Promise.coroutine(function* (urlA, urlB) {
    var resultA = yield http.getAsync(urlA);
    var resultB = yield http.getAsync(urlB);
});
```

The problem with the above is that the requests are not done in parallel. It will completely wait for request A to complete before even starting request B. In the array syntax both requests fire off at the same time in parallel.

<hr>

#####`Promise.coroutine.addYieldHandler(function handler)` -> `void`

By default you can only yield Promises, Thenables and Arrays inside coroutines. You can use this function to add yielding support for arbitrary types.

For example, if you wanted `yield 500` to be same as `yield Promise.delay(500)`:

```js
Promise.coroutine.addYieldHandler(function(value) {
     if (typeof value === "number") return Promise.delay(value);
});
```

Yield handlers are called when you yield something that is not supported by default. The first yield handler to return a promise or a thenable will be used.
If no yield handler returns a promise or a thenable then an error is raised.

An example of implementing callback support with `addYieldHandler`:

*This is a demonstration of how powerful the feature is and not the recommended usage. For best performance you need to use `promisifyAll` and yield promises directly.*

```js
var Promise = require("bluebird");
var fs = require("fs");

var _ = (function() {
    var promise = null;
    Promise.coroutine.addYieldHandler(function(v) {
        if (v === void 0 && promise != null) {
            return promise;
        }
        promise = null;
    });
    return function() {
        var def = Promise.defer();
        promise = def.promise;
        return def.callback;
    };
})();


var readFileJSON = Promise.coroutine(function* (fileName) {
   var contents = yield fs.readFile(fileName, "utf8", _());
   return JSON.parse(contents);
});
```

An example of implementing thunks support with `addYieldHandler`:

*This is a demonstration of how powerful the feature is and not the recommended usage. For best performance you need to use `promisifyAll` and yield promises directly.*

```js
var Promise = require("bluebird");
var fs = require("fs");

Promise.coroutine.addYieldHandler(function(v) {
    if (typeof v === "function") {
        var def = Promise.defer();
        try { v(def.callback); } catch(e) { def.reject(e); }
        return def.promise;
    }
});

var readFileThunk = function(fileName, encoding) {
    return function(cb) {
        return fs.readFile(fileName, encoding, cb);
    };
};

var readFileJSON = Promise.coroutine(function* (fileName) {
   var contents = yield readFileThunk(fileName, "utf8");
   return JSON.parse(contents);
});
```

##Utility

Functions that could potentially be handy in some situations.

#####`.call(String propertyName [, dynamic arg...])` -> `Promise`

This is a convenience method for doing:

```js
promise.then(function(obj){
    return obj[propertyName].call(obj, arg...);
});
```

<hr>

#####`.get(String propertyName)` -> `Promise`

This is a convenience method for doing:

```js
promise.then(function(obj){
    return obj[propertyName];
});
```

<hr>

#####`.return(dynamic value)` -> `Promise`

Convenience method for:

```js
.then(function() {
   return value;
});
```

in the case where `value` doesn't change its value.

That means `value` is bound at the time of calling `.return()` so this will not work as expected:

```js
function getData() {
    var data;

    return query().then(function(result) {
        data = result;
    }).return(data);
}
```

because `data` is `undefined` at the time `.return` is called.

*For compatibility with earlier ECMAScript version, an alias `.thenReturn()` is provided for `.return()`.*

<hr>

#####`.throw(dynamic reason)` -> `Promise`

Convenience method for:

```js
.then(function() {
   throw reason;
});
```

Same limitations apply as with `.return()`.

*For compatibility with earlier ECMAScript version, an alias `.thenThrow()` is provided for `.throw()`.*

<hr>

#####`.toString()` -> `String`

<hr>

#####`.toJSON()` -> `Object`

This is implicitly called by `JSON.stringify` when serializing the object. Returns a serialized representation of the `Promise`.

<hr>

#####`Promise.noConflict()` -> `Object`

This is relevant to browser environments with no module loader.

Release control of the `Promise` namespace to whatever it was before this library was loaded. Returns a reference to the library namespace so you can attach it to something else.

```html
<!-- the other promise library must be loaded first -->
<script type="text/javascript" src="/scripts/other_promise.js"></script>
<script type="text/javascript" src="/scripts/bluebird_debug.js"></script>
<script type="text/javascript">
//Release control right after
var Bluebird = Promise.noConflict();

//Cast a promise from some other Promise library using the Promise namespace to Bluebird:
var promise = Bluebird.cast(new Promise());
</script>
```

<hr>

#####`Promise.onPossiblyUnhandledRejection(Function handler)` -> `undefined`

Add `handler` as the handler to call when there is a possibly unhandled rejection. The default handler logs the error stack to stderr or `console.error` in browsers.

```js
Promise.onPossiblyUnhandledRejection(function(e, promise){
    throw e;
});
```

Passing no value or a non-function will have the effect of removing any kind of handling for possibly unhandled rejections.

<hr>

#####`Promise.onUnhandledRejectionHandled(Function handler)` -> `undefined`

Add `handler` as the handler to call when a rejected promise that was reported as "possibly unhandled rejection" became handled.

Together with `onPossiblyUnhandledRejection` these hooks can be used to implement a debugger that will show a list
of unhandled promise rejections updated in real time as promises become handled.

For example:

```js
var unhandledPromises = [];
Promise.onPossiblyUnhandledRejection(function(reason, promise) {
    unhandledPromises.push(promise);
    //Update some debugger UI
});

Promise.onUnhandledRejectionHandled(function(promise) {
    var index = unhandledPromises.indexOf(promise);
    unhandledPromises.splice(index, 1);
    //Update the debugger UI
});
```

The default approach of bluebird is to immediately log the stack trace upon possibly unhandled rejection. For majority of applications
this will work perfectly, however for some it will give false positives. Such applications can then use the hooks to implement
a more suitable error handling scheme. Any scheme can be implemented on top of these hooks, e.g.:

- Immediate logging to stderr (the default)
- Logging after the promise became GCd (requires a native node.js module)
- Showing a live list of rejected promises
- ...

<hr>

##Collections

Methods of `Promise` instances and core static methods of the Promise class to deal with
collections of promises or mixed promises and values.

#####`.all()` -> `Promise`

Same as calling [Promise.all\(thisPromise\)](#promiseallarraydynamicpromise-values---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>

#####`.props()` -> `Promise`

Same as calling [Promise.props\(thisPromise\)](#promisepropsobjectpromise-object---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>

#####`.settle()` -> `Promise`

Same as calling [Promise.settle\(thisPromise\)](#promisesettlearraydynamicpromise-values---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>

#####`.any()` -> `Promise`

Same as calling [Promise.any\(thisPromise\)](#promiseanyarraydynamicpromise-values---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>

#####`.race()` -> `Promise`

Same as calling [Promise.race\(thisPromise\)](#promiseracearraypromise-promises---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>


#####`.some(int count)` -> `Promise`

Same as calling [Promise.some\(thisPromise, count\)](#promisesomearraydynamicpromise-values-int-count---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>

#####`.spread([Function fulfilledHandler] [, Function rejectedHandler ])` -> `Promise`

Like calling `.then`, but the fulfillment value or rejection reason is assumed to be an array, which is flattened to the formal parameters of the handlers.

```js
Promise.all([task1, task2, task3]).spread(function(result1, result2, result3){

});
```

Normally when using `.then` the code would be like:

```js
Promise.all([task1, task2, task3]).then(function(results){
    var result1 = results[0];
    var result2 = results[1];
    var result3 = results[2];
});
```

This is useful when the `results` array contains items that are not conceptually items of the same list.

<hr>

#####`.map(Function mapper)` -> `Promise`

Same as calling [Promise.map\(thisPromise, mapper\)](#promisemaparraydynamicpromise-values-function-mapper---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>

#####`.reduce(Function reducer [, dynamic initialValue])` -> `Promise`

Same as calling [Promise.reduce\(thisPromise, Function reducer, initialValue\)](#promisereducearraydynamicpromise-values-function-reducer--dynamic-initialvalue---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

<hr>

#####`.filter(Function filterer)` -> `Promise`

Same as calling [`Promise.filter(thisPromise, filterer)`](#promisefilterarraydynamicpromise-values-function-filterer---promise). With the exception that if this promise is [bound](#binddynamic-thisarg---promise) to a value, the returned promise is bound to that value too.

In this example, a list of websites are pinged with 100ms timeout. [`.settle()`](#settle---promise) is used to wait until all pings are either fulfilled or rejected. Then the settled
list of [`PromiseInspections`](#inspect---promiseinspection) is filtered for those that fulfilled (responded in under 100ms) and [`mapped`](#promisemaparraydynamicpromise-values-function-mapper---promise) to the actual fulfillment value.

```js
pingWebsitesAsync({timeout: 100}).settle()
.filter(function(inspection){
    return inspection.isFulfilled();
})
.map(function(inspection){
    return inspection.value();
})
.then(function(websites){
   //List of website names which answered
});
```

The above pattern is actually reusable and can be captured in a method:

```js
Promise.prototype.settledWithFulfill = function() {
    return this.settle()
        .filter(function(inspection){
            return inspection.isFulfilled();
        })
        .map(function(inspection){
            return inspection.value();
        });
};
```

<hr>

#####`Promise.all(Array<dynamic>|Promise values)` -> `Promise`

Given an array, or a promise of an array, which contains promises (or a mix of promises and values) return a promise that is fulfilled when all the items in the array are fulfilled. The promise's fulfillment value is an array with fulfillment values at respective positions to the original array. If any promise in the array rejects, the returned promise is rejected with the rejection reason.

In this example we create a promise that is fulfilled only when the pictures, comments and tweets are all loaded.

```js
Promise.all([getPictures(), getComments(), getTweets()]).then(function(results){
    //Everything loaded and good to go
    var pictures = results[0];
    var comments = results[1];
    var tweets = results[2];
}).catch(function(e){
    alertAsync("error when getting your stuff");
});
```

See [`.spread()`](#spreadfunction-fulfilledhandler--function-rejectedhandler----promise) for a more convenient way to extract the fulfillment values.

*The original array is not modified. The input array sparsity is retained in the resulting array.*

<hr>

#####`Promise.props(Object|Promise object)` -> `Promise`

Like [`Promise.all`](#promiseallarraydynamic-values---promise) but for object properties instead of array items. Returns a promise that is fulfilled when all the properties of the object are fulfilled. The promise's fulfillment value is an object with fulfillment values at respective keys to the original object. If any promise in the object rejects, the returned promise is rejected with the rejection reason.

If `object` is a trusted `Promise`, then it will be treated as a promise for object rather than for its properties. All other objects are treated for their properties as is returned by `Object.keys` - the object's own enumerable properties.

```js
Promise.props({
    pictures: getPictures(),
    comments: getComments(),
    tweets: getTweets()
}).then(function(result){
    console.log(result.tweets, result.pictures, result.comments);
});
```

Note that if you have no use for the result object other than retrieving the properties, it is more convenient to use [`Promise.all`](#promiseallarraydynamic-values---promise) and [`.spread()`](#spreadfunction-fulfilledhandler--function-rejectedhandler----promise):

```js
Promise.all([getPictures(), getComments(), getTweets()])
.spread(function(pictures, comments, tweets) {
    console.log(pictures, comments, tweets);
});
```

*The original object is not modified.*

<hr>

#####`Promise.settle(Array<dynamic>|Promise values)` -> `Promise`

Given an array, or a promise of an array, which contains promises (or a mix of promises and values) return a promise that is fulfilled when all the items in the array are either fulfilled or rejected. The fulfillment value is an array of [`PromiseInspection`](#inspect---promiseinspection) instances at respective positions in relation to the input array.

*The original array is not modified. The input array sparsity is retained in the resulting array.*

<hr>

#####`Promise.any(Array<dynamic>|Promise values)` -> `Promise`

Like [`Promise.some()`](#someint-count---promise), with 1 as `count`. However, if the promise fulfills, the fulfillment value is not an array of 1 but the value directly.

<hr>

#####`Promise.race(Array|Promise promises)` -> `Promise`

Given an array, or a promise of an array, which contains promises (or a mix of promises and values) return a promise that is fulfilled or rejected as soon as a promise in the array is fulfilled or rejected with the respective rejection reason or fulfillment value.

Example of implementing a timeout in terms of `Promise.race`:

```js
var Promise = require("bluebird");
var fs = Promise.promisifyAll(require("fs"));

function delay(ms) {
    return new Promise(function (v) {
        setTimeout(v, ms);
    });
}

function timeout(promise, time) {
    var timeout = delay(time).then(function () {
        throw new Promise.TimeoutError("Operation timed out after " + time + " ms");
    });

    return Promise.race([promise, timeout]);
}

timeout(fs.readFileAsync("slowfile.txt"), 300).then(function (contents) {
    console.log("Here are the contents", contents);
}).
catch(Promise.TimeoutError, function (e) {
    console.error("Sorry retrieving file took too long");
});
```

**Note** If you pass empty array or a sparse array with no values, or a promise/thenable for such, it will be forever pending.

<hr>

#####`Promise.some(Array<dynamic>|Promise values, int count)` -> `Promise`

Initiate a competetive race between multiple promises or values (values will become immediately fulfilled promises). When `count` amount of promises have been fulfilled, the returned promise is fulfilled with an array that contains the fulfillment values of the winners in order of resolution.

This example pings 4 nameservers, and logs the fastest 2 on console:

```js
Promise.some([
    ping("ns1.example.com"),
    ping("ns2.example.com"),
    ping("ns3.example.com"),
    ping("ns4.example.com")
], 2).spread(function(first, second) {
    console.log(first, second);
});
```

If too many promises are rejected so that the promise can never become fulfilled, it will be immediately rejected with an array of rejection reasons in the order they were thrown in.

*The original array is not modified.*

<hr>

#####`Promise.join([dynamic value...])` -> `Promise`

Like [`Promise.all()`](#promiseallarraydynamic-values---promise) but instead of having to pass an array, the array is generated from the passed variadic arguments.

So instead of:

```js
Promise.all([a, b]).spread(function(aResult, bResult) {

});
```

You can do:

```js
Promise.join(a, b).spread(function(aResult, bResult) {

});
```

<hr>

#####`Promise.map(Array<dynamic>|Promise values, Function mapper)` -> `Promise`

Map an array, or a promise of an array, which contains a promises (or a mix of promises and values) with the given `mapper` function with the signature `(item, index, arrayLength)` where `item` is the resolved value of a respective promise in the input array. If any promise in the input array is rejected the returned promise is rejected as well.

If the `mapper` function returns promises or thenables, the returned promise will wait for all the mapped results to be resolved as well.

*(TODO: an example where this is useful)*

*The original array is not modified.*

<hr>

#####`Promise.reduce(Array<dynamic>|Promise values, Function reducer [, dynamic initialValue])` -> `Promise`

Reduce an array, or a promise of an array, which contains a promises (or a mix of promises and values) with the given `reducer` function with the signature `(total, current, index, arrayLength)` where `item` is the resolved value of a respective promise in the input array. If any promise in the input array is rejected the returned promise is rejected as well.

If the reducer function returns a promise or a thenable, the result for the promise is awaited for before continuing with next iteration.

Read given files sequentially while summing their contents as an integer. Each file contains just the text `10`.

```js
Promise.reduce(["file1.txt", "file2.txt", "file3.txt"], function(total, fileName) {
    return fs.readFileAsync(fileName, "utf8").then(function(contents) {
        return total + parseInt(contents, 10);
    });
}, 0).then(function(total) {
    //Total is 30
});
```

*The original array is not modified. If `intialValue` is `undefined` (or a promise that resolves to `undefined`) and the array contains only 1 item, the callback will not be called and `undefined` is returned. If the array is empty, the callback will not be called and `initialValue` is returned (which may be `undefined`).*

<hr>

#####`Promise.filter(Array<dynamic>|Promise values, Function filterer)` -> `Promise`

Filter an array, or a promise of an array, which contains a promises (or a mix of promises and values) with the given `filterer` function with the signature `(item, index, arrayLength)` where `item` is the resolved value of a respective promise in the input array. If any promise in the input array is rejected the returned promise is rejected as well.

The return values from the filtered functions are coerced to booleans, with the exception of promises and thenables which are awaited for their eventual result.

[See the instance method `.filter()` for an example.](#filterfunction-filterer---promise)

*The original array is not modified.

<hr>
