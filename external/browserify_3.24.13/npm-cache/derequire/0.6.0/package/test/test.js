var should = require('chai').should();
var derequire = require('../');
var fs = require("fs");
var crypto = require('crypto');
function hash(data){
  return crypto.createHash('sha512').update(data).digest('base64');
}
var compare = hash(fs.readFileSync('./test/pouchdb.dereq.js', {encoding: 'utf8'}));
describe('derequire', function(){
  it('should work', function(){
    var exampleText = "var x=function(require,module,exports){var process=require(\"__browserify_process\");var requireText = \"require\";}";
    derequire(exampleText).should.equal("var x=function(_dereq_,module,exports){var process=_dereq_(\"__browserify_process\");var requireText = \"require\";}");
  });
  it('should only replace arguments and calls',function(){
    var exampleText = "function x(require,module,exports){var process=require(\"__browserify_process\");var requireText = {}; requireText.require = \"require\";(function(){var require = 'blah';}())}";
    derequire(exampleText).should.equal("function x(_dereq_,module,exports){var process=_dereq_(\"__browserify_process\");var requireText = {}; requireText.require = \"require\";(function(){var require = 'blah';}())}");
  });
  it('should handle top level return statments', function(){
    var exampleText = 'return (function(require){return require();}(function(){return "sentinel";}));';
    derequire(exampleText).should.equal('return (function(_dereq_){return _dereq_();}(function(){return "sentinel";}));');
  });
  it('should work with a comment on the end', function(){
    var exampleText = 'var x=function(require,module,exports){var process=require("__browserify_process");var requireText = "require";}//lala';
    derequire(exampleText).should.equal('var x=function(_dereq_,module,exports){var process=_dereq_("__browserify_process");var requireText = "require";}//lala');
  });
  it('should throw an error if you try to change things of different sizes', function(){
    should.throw(function(){
      derequire('lalalalla', 'la');
    });
  });
  it("should return the code back if it can't parse it", function(){
    derequire("/*").should.equal("/*");
  });
  it('should work on something big', function(done){
    fs.readFile('./test/pouchdb.js', {encoding:'utf8'}, function(err, data){
      if(err){
        done(err);
      }
      var transformed = derequire(data);
      hash(transformed).should.equal(compare);
      done();
    });
  });
  it('should not fail on attribute lookups', function(){
    var txt = 'var x=function(require,module,exports){'
        + 'var W=require("stream").Writable;'
        + '}'
    ;
    var expected = 'var x=function(_dereq_,module,exports){'
        + 'var W=_dereq_("stream").Writable;'
        + '}'
    ;
    derequire(txt).should.equal(expected);
  });
});
