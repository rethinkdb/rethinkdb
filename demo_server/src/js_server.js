goog.require("Query");
goog.require("goog.proto2.WireFormatSerializer");
goog.provide("demo_server");
/*
CryptoJS v3.0.2
code.google.com/p/crypto-js
(c) 2009-2012 by Jeff Mott. All rights reserved.
code.google.com/p/crypto-js/wiki/License
*/
var CryptoJS=CryptoJS||function(i,m){var p={},h=p.lib={},n=h.Base=function(){function a(){}return{extend:function(b){a.prototype=this;var c=new a;b&&c.mixIn(b);c.$super=this;return c},create:function(){var a=this.extend();a.init.apply(a,arguments);return a},init:function(){},mixIn:function(a){for(var c in a)a.hasOwnProperty(c)&&(this[c]=a[c]);a.hasOwnProperty("toString")&&(this.toString=a.toString)},clone:function(){return this.$super.extend(this)}}}(),o=h.WordArray=n.extend({init:function(a,b){a=
this.words=a||[];this.sigBytes=b!=m?b:4*a.length},toString:function(a){return(a||e).stringify(this)},concat:function(a){var b=this.words,c=a.words,d=this.sigBytes,a=a.sigBytes;this.clamp();if(d%4)for(var f=0;f<a;f++)b[d+f>>>2]|=(c[f>>>2]>>>24-8*(f%4)&255)<<24-8*((d+f)%4);else if(65535<c.length)for(f=0;f<a;f+=4)b[d+f>>>2]=c[f>>>2];else b.push.apply(b,c);this.sigBytes+=a;return this},clamp:function(){var a=this.words,b=this.sigBytes;a[b>>>2]&=4294967295<<32-8*(b%4);a.length=i.ceil(b/4)},clone:function(){var a=
n.clone.call(this);a.words=this.words.slice(0);return a},random:function(a){for(var b=[],c=0;c<a;c+=4)b.push(4294967296*i.random()|0);return o.create(b,a)}}),q=p.enc={},e=q.Hex={stringify:function(a){for(var b=a.words,a=a.sigBytes,c=[],d=0;d<a;d++){var f=b[d>>>2]>>>24-8*(d%4)&255;c.push((f>>>4).toString(16));c.push((f&15).toString(16))}return c.join("")},parse:function(a){for(var b=a.length,c=[],d=0;d<b;d+=2)c[d>>>3]|=parseInt(a.substr(d,2),16)<<24-4*(d%8);return o.create(c,b/2)}},g=q.Latin1={stringify:function(a){for(var b=
a.words,a=a.sigBytes,c=[],d=0;d<a;d++)c.push(String.fromCharCode(b[d>>>2]>>>24-8*(d%4)&255));return c.join("")},parse:function(a){for(var b=a.length,c=[],d=0;d<b;d++)c[d>>>2]|=(a.charCodeAt(d)&255)<<24-8*(d%4);return o.create(c,b)}},j=q.Utf8={stringify:function(a){try{return decodeURIComponent(escape(g.stringify(a)))}catch(b){throw Error("Malformed UTF-8 data");}},parse:function(a){return g.parse(unescape(encodeURIComponent(a)))}},k=h.BufferedBlockAlgorithm=n.extend({reset:function(){this._data=o.create();
this._nDataBytes=0},_append:function(a){"string"==typeof a&&(a=j.parse(a));this._data.concat(a);this._nDataBytes+=a.sigBytes},_process:function(a){var b=this._data,c=b.words,d=b.sigBytes,f=this.blockSize,e=d/(4*f),e=a?i.ceil(e):i.max((e|0)-this._minBufferSize,0),a=e*f,d=i.min(4*a,d);if(a){for(var g=0;g<a;g+=f)this._doProcessBlock(c,g);g=c.splice(0,a);b.sigBytes-=d}return o.create(g,d)},clone:function(){var a=n.clone.call(this);a._data=this._data.clone();return a},_minBufferSize:0});h.Hasher=k.extend({init:function(){this.reset()},
reset:function(){k.reset.call(this);this._doReset()},update:function(a){this._append(a);this._process();return this},finalize:function(a){a&&this._append(a);this._doFinalize();return this._hash},clone:function(){var a=k.clone.call(this);a._hash=this._hash.clone();return a},blockSize:16,_createHelper:function(a){return function(b,c){return a.create(c).finalize(b)}},_createHmacHelper:function(a){return function(b,c){return l.HMAC.create(a,c).finalize(b)}}});var l=p.algo={};return p}(Math);
(function(){var i=CryptoJS,m=i.lib,p=m.WordArray,m=m.Hasher,h=[],n=i.algo.SHA1=m.extend({_doReset:function(){this._hash=p.create([1732584193,4023233417,2562383102,271733878,3285377520])},_doProcessBlock:function(o,i){for(var e=this._hash.words,g=e[0],j=e[1],k=e[2],l=e[3],a=e[4],b=0;80>b;b++){if(16>b)h[b]=o[i+b]|0;else{var c=h[b-3]^h[b-8]^h[b-14]^h[b-16];h[b]=c<<1|c>>>31}c=(g<<5|g>>>27)+a+h[b];c=20>b?c+((j&k|~j&l)+1518500249):40>b?c+((j^k^l)+1859775393):60>b?c+((j&k|j&l|k&l)-1894007588):c+((j^k^l)-
899497514);a=l;l=k;k=j<<30|j>>>2;j=g;g=c}e[0]=e[0]+g|0;e[1]=e[1]+j|0;e[2]=e[2]+k|0;e[3]=e[3]+l|0;e[4]=e[4]+a|0},_doFinalize:function(){var i=this._data,h=i.words,e=8*this._nDataBytes,g=8*i.sigBytes;h[g>>>5]|=128<<24-g%32;h[(g+64>>>9<<4)+15]=e;i.sigBytes=4*h.length;this._process()}});i.SHA1=m._createHelper(n);i.HmacSHA1=m._createHmacHelper(n)})();
(function() {
  var Builtin, BuiltinFilter, CreateTable, DemoServer, Helpers, Mapping, MetaQuery, Predicate, Query, ReadQuery, TableRef, Term, TermCall, TermGetByKey, TermTable, VarTermTuple, WriteQuery, WriteQueryInsert, WriteQueryPointDelete, WriteQueryPointUpdate,
    __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };

  Helpers = (function() {

    function Helpers() {
      this.compare = __bind(this.compare, this);

      this.generate_uuid = __bind(this.generate_uuid, this);

    }

    Helpers.prototype.generate_uuid = function() {
      var result;
      result = (Math.random() + '').slice(2) + (Math.random() + '').slice(2);
      result = CryptoJS.SHA1(result) + '';
      result = result.slice(0, 8) + '-' + result.slice(8, 12) + '-' + result.slice(12, 16) + '-' + result.slice(16, 20) + '-' + result.slice(20, 32);
      return result;
    };

    Helpers.prototype.compare = function(a, b) {
      var i, result, type_a, type_b, value, _i, _len;
      value = {
        array: 1,
        boolean: 2,
        "null": 3,
        number: 4,
        object: 5,
        string: 6
      };
      if (typeof a === typeof b) {
        switch (typeof a) {
          case 'array':
            for (i = _i = 0, _len = a.length; _i < _len; i = ++_i) {
              value = a[i];
              result = this.compare(a[i], b[i]);
              if (result !== 0) {
                return result;
              }
            }
            return len(a) - len(b);
          case 'boolean':
            if (a === b) {
              return 0;
            } else if (a === false) {
              return -1;
            } else {
              return 1;
            }
            break;
          case 'object':
            if (a === null && b === null) {
              return 0;
            } else {
              return {
                error: true
              };
            }
            break;
          case 'number':
            return a - b;
          case 'string':
            if (a > b) {
              return 1;
            } else if (a < b) {
              return -1;
            } else {
              return 0;
            }
        }
      } else {
        type_a = typeof a;
        if (type_a === 'object' && a !== null) {
          return {
            error: true
          };
        }
        type_b = typeof b;
        if (type_b === 'object' && b !== null) {
          return {
            error: true
          };
        }
        return value[type_a] - value[type_b];
      }
    };

    return Helpers;

  })();

  Builtin = (function() {

    function Builtin(data) {
      this.data = data;
    }

    Builtin.prototype.evaluate = function(server, args) {
      var builtin_filter, comparison, response, result, term, term1, term1_value, term2, term2_value, term_raw, type, _i, _j, _len, _len1, _ref, _ref1;
      type = this.data.getType();
      switch (type) {
        case 1:
          response = new Response;
          if (args[0].getType() !== 9) {
            response.setStatusCode(103);
            response.setErrorMessage('Not can only be called on a boolean');
            return response;
          }
          response.setStatusCode(1);
          response.addResponse(!args[0].getValuebool());
          return response;
        case 14:
          response = new Response;
          if ((args[0].getType() !== 6 && args[0].getType() !== 10) || (args[1].getType() !== 6 && args[1].getType() !== 10) || (args[0].getType() !== args[1].getType())) {
            response.setStatusCode(103);
            response.setErrorMessage('Can only ADD numbers with number and arrays with arrays');
            return response;
          }
          if (args[0].getType() === 6) {
            result = args[0].getNumber() + args[1].getNumber();
          } else if (args[0].getType() === 10) {
            result = [];
            _ref = args[0].arrayArray();
            for (_i = 0, _len = _ref.length; _i < _len; _i++) {
              term_raw = _ref[_i];
              term = new Term(term_raw);
              result.push(JSON.parse(term.evaluate(server).getResponse()));
            }
            _ref1 = args[1].arrayArray();
            for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
              term_raw = _ref1[_j];
              term = new Term(term_raw);
              result.push(JSON.parse(term.evaluate(server).getResponse()));
            }
          }
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(result));
          return response;
        case 15:
          response = new Response;
          if (args[0].getType() !== 6 || args[1].getType() !== 6) {
            response.setStatusCode(103);
            response.setErrorMessage('All operands to SUBSTRACT must be numbers');
            return response;
          }
          result = args[0].getNumber() - args[1].getNumber();
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(result));
          return response;
        case 16:
          response = new Response;
          if (args[0].getType() !== 6 || args[1].getType() !== 6) {
            response.setStatusCode(103);
            response.setErrorMessage('All operands to MULTIPLY must be numbers');
            return response;
          }
          result = args[0].getNumber() * args[1].getNumber();
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(result));
          return response;
        case 17:
          response = new Response;
          if (args[0].getType() !== 6 || args[1].getType() !== 6) {
            response.setStatusCode(103);
            response.setErrorMessage('All operands to DIVIDE  must be numbers');
            return response;
          }
          result = args[0].getNumber() / args[1].getNumber();
          result = parseFloat(result.toFixed(6));
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(result));
          return response;
        case 18:
          response = new Response;
          if (args[0].getType() !== 6 || args[1].getType() !== 6) {
            response.setStatusCode(103);
            response.setErrorMessage('All operands to DIVIDE  must be numbers');
            return response;
          }
          result = args[0].getNumber() % args[1].getNumber();
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(result));
          return response;
        case 19:
          term1 = new Term(args[0]);
          term1_value = JSON.parse(term1.evaluate(server).getResponse());
          term2 = new Term(args[1]);
          term2_value = JSON.parse(term2.evaluate(server).getResponse());
          response = new Response();
          comparison = this.data.getComparison();
          result = Helpers.prototype.compare(term1_value, term2_value);
          switch (comparison) {
            case 1:
              response.setStatusCode(1);
              response.addResponse(JSON.stringify(result === 0));
              break;
            case 2:
              response.setStatusCode(1);
              response.addResponse(JSON.stringify(result !== 0));
              break;
            case 3:
              response.setStatusCode(1);
              response.addResponse(JSON.stringify(result < 0));
              break;
            case 4:
              response.setStatusCode(1);
              response.addResponse(JSON.stringify(result <= 0));
              break;
            case 5:
              response.setStatusCode(1);
              response.addResponse(JSON.stringify(result > 0));
              break;
            case 6:
              response.setStatusCode(1);
              response.addResponse(JSON.stringify(result >= 0));
          }
          return response;
        case 20:
          builtin_filter = new BuiltinFilter(this.data.getFilter());
          return builtin_filter.evaluate(server, args);
        case 35:
          term1 = new Term(args[0]);
          term1_value = JSON.parse(term1.evaluate(server).getResponse());
          term2 = new Term(args[1]);
          term2_value = JSON.parse(term2.evaluate(server).getResponse());
          response = new Response();
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(term1_value || term2_value));
          return response;
        case 36:
          term1 = new Term(args[0]);
          term1_value = JSON.parse(term1.evaluate(server).getResponse());
          term2 = new Term(args[1]);
          term2_value = JSON.parse(term2.evaluate(server).getResponse());
          response = new Response();
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(term1_value && term2_value));
          return response;
      }
    };

    return Builtin;

  })();

  BuiltinFilter = (function() {

    function BuiltinFilter(data) {
      this.data = data;
    }

    BuiltinFilter.prototype.evaluate = function(server, args) {
      var predicate;
      predicate = new Predicate(this.data.getPredicate());
      return predicate.evaluate(server, args);
    };

    return BuiltinFilter;

  })();

  CreateTable = (function() {

    function CreateTable(data) {
      this.evaluate = __bind(this.evaluate, this);
      this.data = data;
    }

    CreateTable.prototype.evaluate = function(server) {
      var cache_size, datacenter, options, primary_key, table_ref;
      datacenter = this.data.getDatacenter();
      table_ref = new TableRef(this.data.getTableRef());
      primary_key = this.data.getPrimaryKey();
      cache_size = this.data.getCacheSize();
      options = {
        datacenter: (datacenter != null ? datacenter : void 0),
        primary_key: (primary_key != null ? primary_key : void 0),
        cache_size: (cache_size != null ? cache_size : void 0)
      };
      return table_ref.create(server, options);
    };

    return CreateTable;

  })();

  Mapping = (function() {

    function Mapping(data) {
      this.data = data;
    }

    Mapping.prototype.evaluate = function(server) {
      var term;
      term = new Term(this.data.getBody());
      return term.evaluate();
    };

    return Mapping;

  })();

  MetaQuery = (function() {

    function MetaQuery(data) {
      this.data = data;
    }

    MetaQuery.prototype.evaluate = function(server) {
      var create_table, database, db_name, response, result, table, table_to_drop, type;
      type = this.data.getType();
      switch (type) {
        case 1:
          db_name = this.data.getDbName();
          if (/[a-zA-Z0-9_]+/.test(db_name) === false) {
            response = new Response;
            response.setStatusCode(102);
            response.setErrorMessage("Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.).");
            return response;
          }
          if (db_name in server) {
            response = new Response;
            response.setStatusCode(103);
            response.setErrorMessage("Error: Error during operation `CREATE_DB " + db_name + "`: Entry already exists.");
            return response;
          }
          server[db_name] = {};
          response = new Response;
          response.setStatusCode(0);
          return response;
        case 2:
          db_name = this.data.getDbName();
          if (/[a-zA-Z0-9_]+/.test(db_name) === false) {
            response = new Response;
            response.setStatusCode(102);
            response.setErrorMessage("Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.).");
            return response;
          }
          if (!(db_name in server)) {
            response = new Response;
            response.setStatusCode(103);
            response.setErrorMessage("Error: Error during operation `DROP_DB " + db_name + "`: No entry with that name");
            return response;
          }
          delete server[db_name];
          response = new Response;
          response.setStatusCode(0);
          return response;
        case 3:
          response = new Response;
          result = [];
          for (database in server) {
            response.addResponse(JSON.stringify(database));
          }
          response.setStatusCode(3);
          return response;
        case 4:
          create_table = new CreateTable(this.data.getCreateTable());
          return create_table.evaluate(server);
        case 5:
          table_to_drop = new TableRef(this.data.getDropTable());
          return table_to_drop.drop(server);
        case 6:
          response = new Response;
          result = [];
          for (table in server[this.data.getDbName()]) {
            response.addResponse(JSON.stringify(table));
          }
          response.setStatusCode(3);
          return response;
      }
    };

    return MetaQuery;

  })();

  Predicate = (function() {

    function Predicate(data) {
      this.data = data;
    }

    Predicate.prototype.evaluate = function(server, args_table) {
      var term;
      term = new Term(this.data.getBody());
      return term.evaluate(server, args_table);
    };

    return Predicate;

  })();

  ReadQuery = (function() {

    function ReadQuery(data) {
      this.evaluate = __bind(this.evaluate, this);
      this.data = data;
    }

    ReadQuery.prototype.evaluate = function(server) {
      var term;
      term = new Term(this.data.getTerm());
      return term.evaluate(server);
    };

    return ReadQuery;

  })();

  Query = (function() {

    function Query(data) {
      this.evaluate = __bind(this.evaluate, this);
      this.data = data;
    }

    Query.prototype.evaluate = function(server) {
      var meta_query, read_query, type, write_query;
      type = this.data.getType();
      switch (type) {
        case 1:
          read_query = new ReadQuery(this.data.getReadQuery());
          return read_query.evaluate(server);
        case 2:
          write_query = new WriteQuery(this.data.getWriteQuery());
          return write_query.write(server);
        case 5:
          meta_query = new MetaQuery(this.data.getMetaQuery());
          return meta_query.evaluate(server);
      }
    };

    return Query;

  })();

  TableRef = (function() {

    function TableRef(data) {
      this.drop = __bind(this.drop, this);

      this.create = __bind(this.create, this);

      this.get_primary_key = __bind(this.get_primary_key, this);

      this.get_use_outdated = __bind(this.get_use_outdated, this);

      this.get_table_name = __bind(this.get_table_name, this);

      this.get_db_name = __bind(this.get_db_name, this);
      this.data = data;
    }

    TableRef.prototype.get_db_name = function() {
      return this.data.getDbName();
    };

    TableRef.prototype.get_table_name = function() {
      return this.data.getTableName();
    };

    TableRef.prototype.get_use_outdated = function() {
      return this.data.getUseOutdated();
    };

    TableRef.prototype.get_primary_key = function(server) {
      var db_name, table_name;
      db_name = this.data.getDbName();
      table_name = this.data.getTableName();
      return server[db_name][table_name]['options']['primary_key'];
    };

    TableRef.prototype.create = function(server, options) {
      var db_name, response, table_name;
      db_name = this.data.getDbName();
      table_name = this.data.getTableName();
      if (/[a-zA-Z0-9_]+/.test(db_name) === false || /[a-zA-Z0-9_]+/.test(table_name) === false) {
        response = new Response;
        response.setStatusCode(102);
        response.setErrorMessage("Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.).");
        return response;
      }
      if (!db_name in server) {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Error: Error during operation `FIND_DATABASE " + db_name + "`: No entry with that name");
        return response;
      } else if (table_name in server[db_name]) {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Error: Error during operation `CREATE_TABLE " + table_name + "`: Entry already exists");
        return response;
      } else {
        if (!(options.datacenter != null)) {
          options.datacenter = null;
        }
        if (!(options.primary_key != null)) {
          options.primary_key = 'id';
        }
        if (!(options.cache_size != null)) {
          options.cache_size = 1024 * 1024 * 1024;
        }
        server[db_name][table_name] = {
          options: options,
          data: {}
        };
        response = new Response;
        response.setStatusCode(0);
        return response;
      }
    };

    TableRef.prototype.drop = function(server) {
      var db_name, response, table_name;
      db_name = this.data.getDbName();
      table_name = this.data.getTableName();
      if (!db_name in server) {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Error: Error during operation `FIND_DATABASE " + db_name + "`: No entry with that name");
        return response;
      } else if (!table_name in server[db_name]) {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Error: Error during operation `FIND_TABLE " + table_name + "`: No entry with that name");
        return response;
      } else {
        delete server[db_name][table_name];
        response = new Response;
        response.setStatusCode(0);
        return response;
      }
    };

    TableRef.prototype.evaluate = function(server) {
      var db_name, document, id, response, table_name, _ref;
      db_name = this.data.getDbName();
      table_name = this.data.getTableName();
      if (/[a-zA-Z0-9_]+/.test(db_name) === false || /[a-zA-Z0-9_]+/.test(table_name) === false) {
        response = new Response;
        response.setStatusCode(102);
        response.setErrorMessage("Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.).");
        return response;
      }
      if (!(server[db_name] != null)) {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Error: Error during operation `EVAL_DB " + db_name + "`: No entry with that name");
        return response;
      }
      if (!(server[db_name][table_name] != null)) {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Error: Error during operation `EVAL_TABLE " + db_name + "`: No entry with that name");
        return response;
      }
      response = new Response;
      _ref = server[db_name][table_name]['data'];
      for (id in _ref) {
        document = _ref[id];
        response.addResponse(JSON.stringify(document));
      }
      response.setStatusCode(3);
      return response;
    };

    TableRef.prototype.get_term = function(server, term) {
      var db_name, internal_key, key, response, table_name;
      db_name = this.data.getDbName();
      table_name = this.data.getTableName();
      key = JSON.parse(term.evaluate().getResponse());
      if (typeof key === 'number') {
        internal_key = 'N' + key;
      } else if (typeof key === 'string') {
        internal_key = 'S' + key;
      } else {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Runtime Error: Primary key must be a number or a string, not " + key);
        return response;
      }
      response = new Response;
      if (server[db_name][table_name]['data'][internal_key] != null) {
        response.addResponse(JSON.stringify(server[db_name][table_name]['data'][internal_key]));
      }
      response.setStatusCode(3);
      return response;
    };

    TableRef.prototype.find_table = function(server) {
      var db_name, response, table_name;
      db_name = this.data.getDbName();
      table_name = this.data.getTableName();
      if (!db_name in server) {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Error: Error during operation `FIND_DATABASE " + db_name + "`: No entry with that name");
        return response;
      } else if (!table_name in server[db_name]) {
        response = new Response;
        response.setStatusCode(103);
        response.setErrorMessage("Error: Error during operation `FIND_TABLE " + table_name + "`: No entry with that name");
        return response;
      }
      return null;
    };

    TableRef.prototype.point_delete = function(server, attr_name, attr_value) {
      var db_name, internal_key, response, response_find_table, table_name;
      db_name = this.data.getDbName();
      table_name = this.data.getTableName();
      response_find_table = this.find_table(server);
      if (response_find_table != null) {
        return response_find_table;
      } else {
        if (typeof attr_value === 'number') {
          internal_key = 'N' + attr_value;
        } else if (typeof attr_value === 'string') {
          internal_key = 'S' + attr_value;
        }
        response = new Response;
        if (server[db_name][table_name]['data'][internal_key] != null) {
          delete server[db_name][table_name]['data'][internal_key];
          response.addResponse(JSON.stringify({
            deleted: 1
          }));
          response.setStatusCode(1);
        } else {
          response.addResponse(JSON.stringify({
            deleted: 1
          }));
          response.setStatusCode(1);
        }
        return response;
      }
    };

    TableRef.prototype.point_update = function(server, attr_name, attr_value, mapping_value) {
      var db_name, internal_key, key, response, response_find_table, table_name, value;
      db_name = this.data.getDbName();
      table_name = this.data.getTableName();
      response_find_table = this.find_table(server);
      if (response_find_table != null) {
        return response_find_table;
      } else {
        if (typeof attr_value === 'number') {
          internal_key = 'N' + attr_value;
        } else if (typeof attr_value === 'string') {
          internal_key = 'S' + attr_value;
        }
        response = new Response;
        if (server[db_name][table_name]['data'][internal_key] != null) {
          for (key in mapping_value) {
            value = mapping_value[key];
            server[db_name][table_name]['data'][internal_key][key] = value;
          }
          response.addResponse(JSON.stringify({
            updated: 1,
            skipped: 0,
            errors: 0
          }));
          response.setStatusCode(1);
        } else {
          response.addResponse(JSON.stringify({
            updated: 1,
            skipped: 0,
            errors: 0
          }));
          response.setStatusCode(1);
        }
        return response;
      }
    };

    return TableRef;

  })();

  Term = (function() {

    function Term(data) {
      this.data = data;
    }

    Term.prototype.evaluate = function(server) {
      var new_term, new_var_term_tuple, response, result, term_call, term_data, term_get_by_key, term_table, type, _i, _j, _len, _len1, _ref, _ref1;
      type = this.data.getType();
      switch (type) {
        case 0:
          response = new Response;
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(null));
          return response;
        case 3:
          term_call = new TermCall(this.data.getCall());
          return term_call.evaluate(server);
        case 6:
          response = new Response;
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(this.data.getNumber()));
          return response;
        case 7:
          response = new Response;
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(this.data.getValuestring()));
          return response;
        case 9:
          response = new Response;
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(this.data.getValuebool()));
          return response;
        case 10:
          response = new Response;
          response.setStatusCode(1);
          result = [];
          _ref = this.data.arrayArray();
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            term_data = _ref[_i];
            new_term = new Term(term_data);
            result.push(JSON.parse(new_term.evaluate().getResponse()));
          }
          response.addResponse(JSON.stringify(result));
          return response;
        case 11:
          response = new Response;
          response.setStatusCode(1);
          result = {};
          _ref1 = this.data.objectArray();
          for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
            term_data = _ref1[_j];
            new_var_term_tuple = new VarTermTuple(term_data);
            result[new_var_term_tuple.get_key()] = JSON.parse(new_var_term_tuple.evaluate().getResponse());
          }
          response.addResponse(JSON.stringify(result));
          return response;
        case 12:
          term_get_by_key = new TermGetByKey(this.data.getGetByKey());
          return term_get_by_key.evaluate(server);
        case 13:
          term_table = new TermTable(this.data.getTable());
          return term_table.evaluate(server);
        case 14:
          response = new Response;
          response.setStatusCode(1);
          response.addResponse(JSON.stringify(eval('(function(){' + this.data.getJavascript() + ';})()')));
          return response;
      }
    };

    return Term;

  })();

  TermCall = (function() {

    function TermCall(data) {
      this.data = data;
    }

    TermCall.prototype.evaluate = function(server) {
      var args, builtin;
      builtin = new Builtin(this.data.getBuiltin());
      args = this.data.argsArray();
      return builtin.evaluate(server, args);
    };

    return TermCall;

  })();

  TermGetByKey = (function() {

    function TermGetByKey(data) {
      this.data = data;
    }

    TermGetByKey.prototype.evaluate = function(server) {
      var table_ref, term;
      table_ref = new TableRef(this.data.getTableRef());
      term = new Term(this.data.getKey());
      return table_ref.get_term(server, term);
    };

    return TermGetByKey;

  })();

  TermTable = (function() {

    function TermTable(data) {
      this.data = data;
    }

    TermTable.prototype.evaluate = function(server) {
      var table_ref;
      table_ref = new TableRef(this.data.getTableRef());
      return table_ref.evaluate(server);
    };

    return TermTable;

  })();

  VarTermTuple = (function() {

    function VarTermTuple(data) {
      this.evaluate = __bind(this.evaluate, this);

      this.get_key = __bind(this.get_key, this);
      this.data = data;
    }

    VarTermTuple.prototype.get_key = function() {
      return this.data.getVar();
    };

    VarTermTuple.prototype.evaluate = function(server) {
      var term;
      term = new Term(this.data.getTerm());
      return term.evaluate(server);
    };

    return VarTermTuple;

  })();

  WriteQuery = (function() {

    function WriteQuery(data) {
      this.data = data;
    }

    WriteQuery.prototype.write = function(server) {
      var insert_query, point_delete_query, point_update_query, type;
      type = this.data.getType();
      switch (type) {
        case 4:
          insert_query = new WriteQueryInsert(this.data.getInsert());
          return insert_query.insert(server);
        case 7:
          point_update_query = new WriteQueryPointUpdate(this.data.getPointUpdate());
          return point_update_query.update(server);
        case 8:
          point_delete_query = new WriteQueryPointDelete(this.data.getPointDelete());
          return point_delete_query["delete"](server);
      }
    };

    return WriteQuery;

  })();

  WriteQueryInsert = (function() {

    function WriteQueryInsert(data) {
      this.data = data;
    }

    WriteQueryInsert.prototype.insert = function(server) {
      var data_to_insert, generated_keys, internal_key, json_object, key, new_id, response, response_data, table_ref, term_to_insert, term_to_insert_raw, _i, _len;
      table_ref = new TableRef(this.data.getTableRef());
      data_to_insert = this.data.termsArray();
      response_data = {
        inserted: 0,
        errors: 0
      };
      generated_keys = [];
      for (_i = 0, _len = data_to_insert.length; _i < _len; _i++) {
        term_to_insert_raw = data_to_insert[_i];
        term_to_insert = new Term(term_to_insert_raw);
        json_object = JSON.parse(term_to_insert.evaluate(server).getResponse());
        if (table_ref.get_primary_key(server) in json_object) {
          key = json_object[table_ref.get_primary_key(server)];
          if (typeof key === 'number') {
            internal_key = 'N' + key;
          } else if (typeof key === 'string') {
            internal_key = 'S' + key;
          } else {
            if (!('first_error' in response_data)) {
              response_data.first_error = "Cannot insert row " + (JSON.stringify(json_object, void 0, 1)) + " with primary key " + (JSON.stringify(key, void 0, 1)) + " of non-string, non-number type.";
            }
            response_data.errors++;
            continue;
          }
          if (internal_key in server[table_ref.get_db_name()][table_ref.get_table_name()]['data']) {
            response_data.errors++;
            if (!('first_error' in response_data)) {
              response_data.first_error = "Duplicate primary key " + (table_ref.get_primary_key(server)) + " in " + (JSON.stringify(json_object, void 0, 1));
            }
          } else {
            server[table_ref.get_db_name()][table_ref.get_table_name()]['data'][internal_key] = json_object;
            response_data.inserted++;
          }
        } else {
          new_id = Helpers.prototype.generate_uuid();
          internal_key = 'S' + new_id;
          if (internal_key in server[table_ref.get_db_name()][table_ref.get_table_name()]['data']) {
            response_data.errors++;
            if (!('first_error' in response_data)) {
              response_data.first_error = "Generated key was a duplicate either you've won the uuid lottery or you've intentionnaly tried to predict the keys rdb would generate... in which case well done.";
            }
          } else {
            json_object[table_ref.get_primary_key(server)] = new_id;
            server[table_ref.get_db_name()][table_ref.get_table_name()]['data'][internal_key] = json_object;
            response_data.inserted++;
            generated_keys.push(new_id);
          }
        }
      }
      if (generated_keys.length > 0) {
        response_data.generated_keys = generated_keys;
      }
      response = new Response;
      response.setStatusCode(1);
      response.addResponse(JSON.stringify(response_data));
      return response;
    };

    return WriteQueryInsert;

  })();

  WriteQueryPointDelete = (function() {

    function WriteQueryPointDelete(data) {
      this.data = data;
    }

    WriteQueryPointDelete.prototype["delete"] = function(server) {
      var attr_name, attr_value, table_ref, term;
      table_ref = new TableRef(this.data.getTableRef());
      attr_name = this.data.getAttrname();
      term = new Term(this.data.getKey());
      attr_value = JSON.parse(term.evaluate().getResponse());
      return table_ref.point_delete(server, attr_name, attr_value);
    };

    return WriteQueryPointDelete;

  })();

  WriteQueryPointUpdate = (function() {

    function WriteQueryPointUpdate(data) {
      this.data = data;
    }

    WriteQueryPointUpdate.prototype.update = function(server) {
      var attr_name, attr_value, mapping, mapping_value, table_ref, term;
      table_ref = new TableRef(this.data.getTableRef());
      attr_name = this.data.getAttrname();
      term = new Term(this.data.getKey());
      attr_value = JSON.parse(term.evaluate().getResponse());
      mapping = new Mapping(this.data.getMapping());
      mapping_value = JSON.parse(mapping.evaluate().getResponse());
      return table_ref.point_update(server, attr_name, attr_value, mapping_value);
    };

    return WriteQueryPointUpdate;

  })();

  /*
  response = new Response
  response.setStatusCode 103
  response.setErrorMessage 
  return response
  */


  DemoServer = (function() {

    function DemoServer() {
      this.execute = __bind(this.execute, this);

      this.pb2query = __bind(this.pb2query, this);

      this.set_serializer = __bind(this.set_serializer, this);

      this.set_descriptor = __bind(this.set_descriptor, this);

      this.print_all = __bind(this.print_all, this);
      this.local_server = {};
      this.set_descriptor();
      this.set_serializer();
    }

    DemoServer.prototype.print_all = function() {
      return console.log(this.local_server);
    };

    DemoServer.prototype.set_descriptor = function() {
      return this.descriptor = window.Query.getDescriptor();
    };

    DemoServer.prototype.set_serializer = function() {
      return this.serializer = new goog.proto2.WireFormatSerializer();
    };

    DemoServer.prototype.buffer2pb = function(buffer) {
      var expanded_array;
      expanded_array = new Uint8Array(buffer);
      return expanded_array.subarray(4);
    };

    DemoServer.prototype.pb2query = function(intarray) {
      return this.serializer.deserialize(this.descriptor, intarray);
    };

    DemoServer.prototype.execute = function(data) {
      var data_query, final_response, length, query, response, serialized_response;
      data_query = this.pb2query(this.buffer2pb(data));
      console.log('Server: deserialized query');
      console.log(data_query);
      query = new Query(data_query);
      response = query.evaluate(this.local_server);
      if (response != null) {
        response.setToken(data_query.getToken());
      }
      console.log('Server: response');
      console.log(response);
      serialized_response = this.serializer.serialize(response);
      length = serialized_response.byteLength;
      final_response = new Uint8Array(length + 4);
      final_response[0] = length % 256;
      final_response[1] = Math.floor(length / 256);
      final_response[2] = Math.floor(length / (256 * 256));
      final_response[3] = Math.floor(length / (256 * 256 * 256));
      final_response.set(serialized_response, 4);
      console.log('Server: serialized response');
      console.log(final_response);
      return final_response;
    };

    return DemoServer;

  })();

  window.DemoServer = DemoServer;

}).call(this);
