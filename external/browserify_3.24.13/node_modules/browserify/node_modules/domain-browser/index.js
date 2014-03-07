/*global define:false require:false */
module.exports = (function(){
  // Import Events
  var events = require('events');

  // Export Domain
  var domain = {};
  domain.create = function(){
    var d = new events.EventEmitter();
    d.run = function(fn){
      try {
        fn();
      }
      catch (err) {
        this.emit('error', err);
      }
      return this;
    };
    d.dispose = function(){
      this.removeAllListeners();
      return this;
    };
    return d;
  };
  return domain;
}).call(this);