var Parser = require('jsonparse')
  , Stream = require('stream').Stream

/*

  the value of this.stack that creationix's jsonparse has is weird.

  it makes this code ugly, but his problem is way harder that mine,
  so i'll forgive him.

*/

exports.parse = function (path) {

  var stream = new Stream()
  var parser = new Parser()
  var count = 0
  if(!path || !path.length)
    path = null
  parser.onValue = function () {
    if(!this.root && this.stack.length == 1){
      stream.root = this.value
      }
    if(!path || this.stack.length !== path.length)
      return
    var _path = []
    for( var i = 0; i < (path.length - 1); i++) {
      var key = path[i]
      var c = this.stack[1 + (+i)]

      if(!c) {
        return
      }
      var m = check(key, c.key)
      _path.push(c.key)

       if(!m)
        return

    }
    var c = this

    var key = path[path.length - 1]
      var m = check(key, c.key)
     if(!m)
      return
      _path.push(c.key)

  count ++
  stream.emit('data', this.value[this.key])
  }

  parser._onToken = parser.onToken;

  parser.onToken = function (token, value) {
    parser._onToken(token, value);
    if (this.stack.length === 0) {
      if (stream.root) {
        if(!path)
          stream.emit('data', stream.root)
        stream.emit('root', stream.root, count)
        count = 0;
        stream.root = null;
      }
    }
  }

  parser.onError = function (err) {
    stream.emit('error', err)
  }
  stream.readable = true
  stream.writable = true
  stream.write = function (chunk) {
    if('string' === typeof chunk) {
      if ('undefined' === typeof Buffer) {
        var buf = new Array(chunk.length)
        for (var i = 0; i < chunk.length; i++) buf[i] = chunk.charCodeAt(i)
        chunk = new Int32Array(buf)
      } else {
        chunk = new Buffer(chunk)
      }
    }
    parser.write(chunk)
  }
  stream.end = function (data) {
    if(data)
      stream.write(data)
    stream.emit('end')
  }

  stream.destroy = function () {
    stream.emit('close');
  }

  return stream
}

function check (x, y) {
  if ('string' === typeof x)
    return y == x
  else if (x && 'function' === typeof x.exec)
    return x.exec(y)
  else if ('boolean' === typeof x)
    return x
  else if ('function' === typeof x)
    return x(y)
  return false
}

exports.stringify = function (op, sep, cl) {
  if (op === false){
    op = ''
    sep = '\n'
    cl = ''
  } else if (op == null) {

    op = '[\n'
    sep = '\n,\n'
    cl = '\n]\n'

  }

  //else, what ever you like

  var stream = new Stream ()
    , first = true
    , ended = false
    , anyData = false
  stream.write = function (data) {
    anyData = true
    var json = JSON.stringify(data)
    if(first) { first = false ; stream.emit('data', op + json)}
    else stream.emit('data', sep + json)
  }
  stream.end = function (data) {
    if(ended)
      return
    ended = true
    if(data) stream.write(data)
    if(!anyData) stream.emit('data', op)
    stream.emit('data', cl)

    stream.emit('end')
  }
  stream.writable = true
  stream.readable = true

  return stream
}

exports.stringifyObject = function (op, sep, cl) {
  if (op === false){
    op = ''
    sep = '\n'
    cl = ''
  } else if (op == null) {

    op = '{\n'
    sep = '\n,\n'
    cl = '\n}\n'

  }

  //else, what ever you like

  var stream = new Stream ()
    , first = true
    , ended = false
    , anyData = false
  stream.write = function (data) {
    anyData = true
    var json = JSON.stringify(data[0]) + ':' + JSON.stringify(data[1])
    if(first) { first = false ; stream.emit('data', op + json)}
    else stream.emit('data', sep + json)
  }
  stream.end = function (data) {
    if(ended) return
    ended = true
    if(data) stream.write(data)
    if(!anyData) stream.emit('data', op)
    stream.emit('data', cl)

    stream.emit('end')
  }
  stream.writable = true
  stream.readable = true

  return stream
}
