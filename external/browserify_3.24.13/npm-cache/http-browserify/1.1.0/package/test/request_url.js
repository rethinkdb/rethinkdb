global.window = {
  location: {
    host: 'localhost:8081',
    port: 8081,
    protocol: 'http:'
  }
};

var noop = function() {};
global.window.XMLHttpRequest = function() {
  this.open = noop;
  this.send = noop;
};

var test = require('tape').test;
var http = require('../index.js');


test('Test simple url string', function(t) {
  var url = { path: '/api/foo' };
  var request = http.get(url, noop);

  t.equal( request.uri, 'http://localhost:8081/api/foo', 'Url should be correct');
  t.end();

});


test('Test full url object', function(t) {
  var url = {
    host: "localhost:8081",
    hostname: "localhost",
    href: "http://localhost:8081/api/foo?bar=baz",
    method: "GET",
    path: "/api/foo?bar=baz",
    pathname: "/api/foo",
    port: "8081",
    protocol: "http:",
    query: "bar=baz",
    search: "?bar=baz",
    slashes: true
  };

  var request = http.get(url, noop);

  t.equal( request.uri, 'http://localhost:8081/api/foo?bar=baz', 'Url should be correct');
  t.end();

});


test('Test string as parameters', function(t) {
  var url = '/api/foo';
  var request = http.get(url, noop);

  t.equal( request.uri, 'http://localhost:8081/api/foo', 'Url should be correct');
  t.end();

})