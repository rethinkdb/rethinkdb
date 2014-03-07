**esrefactor** (BSD licensed) is a little helper library for ECMAScript refactoring.

## Usage

With Node.js:

```bash
npm install esrefactor
```

In a browser, include first all the dependents:

```
<!-- esrefactor depends on these libraries -->
<script src="esprima.js"></src>
<script src="estraverse.js"></src>
<script src="escope.js"></src>

<script src="esrefactor.js"></src>
```
## API

Before using the API, a context needs to be created:

```javascript
var ctx = new esrefactor.Context(code);
```

where `code` is the source code.

### Identification

An identifier, whether it is a variable, a function name, or a function
parameter, can be identified using `identify()`. Example:

```javascript
var ctx = new esrefactor.Context('var x = 42; y = x * 2; z = x / 2');
var id = ctx.identify(4);
```

The only argument to `identify` is the zero-based position index.

The return object has 3 (three) properties:

* `identifier`: the syntax node associated with the position
* `declaration`: the declaration syntax node for the identifier
* `references`: an array of all identical references

If there is no declaration for the identifier (e.g. `x = 42`, global
leak), then `declaration` will be _null_.

The resolution of the declaration syntax node and the references array
take into account the identifier scope as defined in the official
ECMAScript 5.1 Specification (ECMA-262).

Note that if there is no identifier in the given position index,
`identify()` will return _undefined_.

### Renaming

An identifier can be renamed using `rename()`. All other identical references
associated with that identifier will be renamed as well, again taking into
account the proper identifier scope. Renaming works for variable declaration,
function name, and function parameter.

For `rename()` to work, it needs to have the identification result
(via `identify`) and the new name for the identifier.

```javascript
var ctx = new esrefactor.Context('var x; x = 42');
var id = ctx.identify(4);
var code = ctx.rename(id, 'answer');
```
In the above example, `code` is `var answer; answer = 42`.

