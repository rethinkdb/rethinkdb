!function(e){"object"==typeof exports?module.exports=e():"function"==typeof define&&define.amd?define(e):"undefined"!=typeof window?window.PouchDB=e():"undefined"!=typeof global?global.PouchDB=e():"undefined"!=typeof self&&(self.PouchDB=e())}(function(){var define,module,exports;return (function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);throw new Error("Cannot find module '"+o+"'")}var f=n[o]={exports:{}};t[o][0].call(f.exports,function(e){var n=t[o][1][e];return s(n?n:e)},f,f.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(_dereq_,module,exports){
/*globals cordova */

"use strict";

var utils = _dereq_('./utils');
var merge = _dereq_('./merge');
var errors = _dereq_('./deps/errors');
var call = utils.call;

/*
 * A generic pouch adapter
 */

// returns first element of arr satisfying callback predicate
function arrayFirst(arr, callback) {
  for (var i = 0; i < arr.length; i++) {
    if (callback(arr[i], i) === true) {
      return arr[i];
    }
  }
  return false;
}

// Wrapper for functions that call the bulkdocs api with a single doc,
// if the first result is an error, return an error
function yankError(callback) {
  return function (err, results) {
    if (err || results[0].error) {
      call(callback, err || results[0]);
    } else {
      call(callback, null, results[0]);
    }
  };
}

// for every node in a revision tree computes its distance from the closest
// leaf
function computeHeight(revs) {
  var height = {};
  var edges = [];
  merge.traverseRevTree(revs, function (isLeaf, pos, id, prnt) {
    var rev = pos + "-" + id;
    if (isLeaf) {
      height[rev] = 0;
    }
    if (prnt !== undefined) {
      edges.push({from: prnt, to: rev});
    }
    return rev;
  });

  edges.reverse();
  edges.forEach(function (edge) {
    if (height[edge.from] === undefined) {
      height[edge.from] = 1 + height[edge.to];
    } else {
      height[edge.from] = Math.min(height[edge.from], 1 + height[edge.to]);
    }
  });
  return height;
}

module.exports = function (Pouch) {
  return function (opts, callback) {
    var api = {};

    var customApi = Pouch.adapters[opts.adapter](opts, function (err, db) {
      if (err) {
        if (callback) {
          callback(err);
        }
        return;
      }

      for (var j in api) {
        if (!db.hasOwnProperty(j)) {
          db[j] = api[j];
        }
      }

      // Don't call Pouch.open for ALL_DBS
      // Pouch.open saves the db's name into ALL_DBS
      if (opts.name === Pouch.prefix + Pouch.ALL_DBS) {
        callback(err, db);
      } else {
        Pouch.open(opts, function (err) {
          callback(err, db);
        });
      }
    });

    var auto_compaction = (opts.auto_compaction === true);

    // wraps a callback with a function that runs compaction after each edit
    function autoCompact(callback) {
      if (!auto_compaction) {
        return callback;
      }
      return function (err, res) {
        if (err) {
          call(callback, err);
        } else {
          var count = res.length;
          var decCount = function () {
            count--;
            if (!count) {
              call(callback, null, res);
            }
          };
          res.forEach(function (doc) {
            if (doc.ok) {
              // TODO: we need better error handling
              compactDocument(doc.id, 1, decCount);
            } else {
              decCount();
            }
          });
        }
      };
    }

    api.post = function (doc, opts, callback) {
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      if (typeof doc !== 'object' || Array.isArray(doc)) {
        return call(callback, errors.NOT_AN_OBJECT);
      }
      return customApi.bulkDocs({docs: [doc]}, opts,
          autoCompact(yankError(callback)));
    };

    api.put = function (doc, opts, callback) {
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      if (typeof doc !== 'object') {
        return call(callback, errors.NOT_AN_OBJECT);
      }
      if (!utils.isValidId(doc._id)) {
        return call(callback, errors.MISSING_ID);
      }
      return customApi.bulkDocs({docs: [doc]}, opts,
          autoCompact(yankError(callback)));
    };

    api.putAttachment = function (docId, attachmentId, rev, blob, type, callback) {
      if (!api.taskqueue.ready()) {
        api.taskqueue.addTask('putAttachment', arguments);
        return;
      }
      if (typeof type === 'function') {
        callback = type;
        type = blob;
        blob = rev;
        rev = null;
      }
      if (typeof type === 'undefined') {
        type = blob;
        blob = rev;
        rev = null;
      }

      function createAttachment(doc) {
        doc._attachments = doc._attachments || {};
        doc._attachments[attachmentId] = {
          content_type: type,
          data: blob
        };
        api.put(doc, callback);
      }

      api.get(docId, function (err, doc) {
        // create new doc
        if (err && err.error === errors.MISSING_DOC.error) {
          createAttachment({_id: docId});
          return;
        }
        if (err) {
          call(callback, err);
          return;
        }

        if (doc._rev !== rev) {
          call(callback, errors.REV_CONFLICT);
          return;
        }

        createAttachment(doc);
      });
    };

    api.removeAttachment = function (docId, attachmentId, rev, callback) {
      api.get(docId, function (err, obj) {
        if (err) {
          call(callback, err);
          return;
        }
        if (obj._rev !== rev) {
          call(callback, errors.REV_CONFLICT);
          return;
        }
        if (!obj._attachments) {
          return call(callback, null);
        }
        delete obj._attachments[attachmentId];
        if (Object.keys(obj._attachments).length === 0) {
          delete obj._attachments;
        }
        api.put(obj, callback);
      });
    };

    api.remove = function (doc, opts, callback) {
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      if (opts === undefined) {
        opts = {};
      }
      opts.was_delete = true;
      var newDoc = {_id: doc._id, _rev: doc._rev};
      newDoc._deleted = true;
      return customApi.bulkDocs({docs: [newDoc]}, opts, yankError(callback));
    };

    api.revsDiff = function (req, opts, callback) {
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      var ids = Object.keys(req);
      var count = 0;
      var missing = {};

      function addToMissing(id, revId) {
        if (!missing[id]) {
          missing[id] = {missing: []};
        }
        missing[id].missing.push(revId);
      }

      function processDoc(id, rev_tree) {
        // Is this fast enough? Maybe we should switch to a set simulated by a map
        var missingForId = req[id].slice(0);
        merge.traverseRevTree(rev_tree, function (isLeaf, pos, revHash, ctx,
          opts) {
            var rev = pos + '-' + revHash;
            var idx = missingForId.indexOf(rev);
            if (idx === -1) {
              return;
            }

            missingForId.splice(idx, 1);
            if (opts.status !== 'available') {
              addToMissing(id, rev);
            }
          });

        // Traversing the tree is synchronous, so now `missingForId` contains
        // revisions that were not found in the tree
        missingForId.forEach(function (rev) {
          addToMissing(id, rev);
        });
      }

      ids.map(function (id) {
        customApi._getRevisionTree(id, function (err, rev_tree) {
          if (err && err.name === 'not_found' && err.message === 'missing') {
            missing[id] = {missing: req[id]};
          } else if (err) {
            return call(callback, err);
          } else {
            processDoc(id, rev_tree);
          }

          if (++count === ids.length) {
            return call(callback, null, missing);
          }
        });
      });
    };

    // compact one document and fire callback
    // by compacting we mean removing all revisions which
    // are further from the leaf in revision tree than max_height
    function compactDocument(docId, max_height, callback) {
      customApi._getRevisionTree(docId, function (err, rev_tree) {
        if (err) {
          return call(callback);
        }
        var height = computeHeight(rev_tree);
        var candidates = [];
        var revs = [];
        Object.keys(height).forEach(function (rev) {
          if (height[rev] > max_height) {
            candidates.push(rev);
          }
        });

        merge.traverseRevTree(rev_tree, function (isLeaf, pos, revHash, ctx, opts) {
          var rev = pos + '-' + revHash;
          if (opts.status === 'available' && candidates.indexOf(rev) !== -1) {
            opts.status = 'missing';
            revs.push(rev);
          }
        });
        customApi._doCompaction(docId, rev_tree, revs, callback);
      });
    }

    // compact the whole database using single document
    // compaction
    api.compact = function (opts, callback) {
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      api.changes({complete: function (err, res) {
        if (err) {
          call(callback); // TODO: silently fail
          return;
        }
        var count = res.results.length;
        if (!count) {
          call(callback);
          return;
        }
        res.results.forEach(function (row) {
          compactDocument(row.id, 0, function () {
            count--;
            if (!count) {
              call(callback);
            }
          });
        });
      }});
    };

    /* Begin api wrappers. Specific functionality to storage belongs in the _[method] */
    api.get = function (id, opts, callback) {
      if (!api.taskqueue.ready()) {
        api.taskqueue.addTask('get', arguments);
        return;
      }
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }

      var leaves = [];
      function finishOpenRevs() {
        var result = [];
        var count = leaves.length;
        if (!count) {
          return call(callback, null, result);
        }
        // order with open_revs is unspecified
        leaves.forEach(function (leaf) {
          api.get(id, {rev: leaf, revs: opts.revs}, function (err, doc) {
            if (!err) {
              result.push({ok: doc});
            } else {
              result.push({missing: leaf});
            }
            count--;
            if (!count) {
              call(callback, null, result);
            }
          });
        });
      }

      if (opts.open_revs) {
        if (opts.open_revs === "all") {
          customApi._getRevisionTree(id, function (err, rev_tree) {
            if (err) {
              // if there's no such document we should treat this
              // situation the same way as if revision tree was empty
              rev_tree = [];
            }
            leaves = merge.collectLeaves(rev_tree).map(function (leaf) {
              return leaf.rev;
            });
            finishOpenRevs();
          });
        } else {
          if (Array.isArray(opts.open_revs)) {
            leaves = opts.open_revs;
            for (var i = 0; i < leaves.length; i++) {
              var l = leaves[i];
              // looks like it's the only thing couchdb checks
              if (!(typeof(l) === "string" && /^\d+-/.test(l))) {
                return call(callback, errors.error(errors.BAD_REQUEST,
                  "Invalid rev format"));
              }
            }
            finishOpenRevs();
          } else {
            return call(callback, errors.error(errors.UNKNOWN_ERROR,
              'function_clause'));
          }
        }
        return; // open_revs does not like other options
      }

      return customApi._get(id, opts, function (err, result) {
        if (err) {
          return call(callback, err);
        }

        var doc = result.doc;
        var metadata = result.metadata;
        var ctx = result.ctx;

        if (opts.conflicts) {
          var conflicts = merge.collectConflicts(metadata);
          if (conflicts.length) {
            doc._conflicts = conflicts;
          }
        }

        if (opts.revs || opts.revs_info) {
          var paths = merge.rootToLeaf(metadata.rev_tree);
          var path = arrayFirst(paths, function (arr) {
            return arr.ids.map(function (x) { return x.id; })
              .indexOf(doc._rev.split('-')[1]) !== -1;
          });

          path.ids.splice(path.ids.map(function (x) {return x.id; })
                          .indexOf(doc._rev.split('-')[1]) + 1);
          path.ids.reverse();

          if (opts.revs) {
            doc._revisions = {
              start: (path.pos + path.ids.length) - 1,
              ids: path.ids.map(function (rev) {
                return rev.id;
              })
            };
          }
          if (opts.revs_info) {
            var pos =  path.pos + path.ids.length;
            doc._revs_info = path.ids.map(function (rev) {
              pos--;
              return {
                rev: pos + '-' + rev.id,
                status: rev.opts.status
              };
            });
          }
        }

        if (opts.local_seq) {
          doc._local_seq = result.metadata.seq;
        }

        if (opts.attachments && doc._attachments) {
          var attachments = doc._attachments;
          var count = Object.keys(attachments).length;
          if (count === 0) {
            return call(callback, null, doc);
          }
          Object.keys(attachments).forEach(function (key) {
            customApi._getAttachment(attachments[key], {encode: true, ctx: ctx}, function (err, data) {
              doc._attachments[key].data = data;
              if (!--count) {
                call(callback, null, doc);
              }
            });
          });
        } else {
          if (doc._attachments) {
            for (var key in doc._attachments) {
              doc._attachments[key].stub = true;
            }
          }
          call(callback, null, doc);
        }
      });
    };

    api.getAttachment = function (docId, attachmentId, opts, callback) {
      if (!api.taskqueue.ready()) {
        api.taskqueue.addTask('getAttachment', arguments);
        return;
      }
      if (opts instanceof Function) {
        callback = opts;
        opts = {};
      }
      customApi._get(docId, opts, function (err, res) {
        if (err) {
          return call(callback, err);
        }
        if (res.doc._attachments && res.doc._attachments[attachmentId]) {
          opts.ctx = res.ctx;
          customApi._getAttachment(res.doc._attachments[attachmentId], opts, callback);
        } else {
          return call(callback, errors.MISSING_DOC);
        }
      });
    };

    api.allDocs = function (opts, callback) {
      if (!api.taskqueue.ready()) {
        api.taskqueue.addTask('allDocs', arguments);
        return;
      }
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      if ('keys' in opts) {
        if ('startkey' in opts) {
          call(callback, errors.error(errors.QUERY_PARSE_ERROR,
            'Query parameter `start_key` is not compatible with multi-get'
          ));
          return;
        }
        if ('endkey' in opts) {
          call(callback, errors.error(errors.QUERY_PARSE_ERROR,
            'Query parameter `end_key` is not compatible with multi-get'
          ));
          return;
        }
      }
      if (typeof opts.skip === 'undefined') {
        opts.skip = 0;
      }

      return customApi._allDocs(opts, callback);
    };

    function processChange(doc, metadata, opts) {
      var changeList = [{rev: doc._rev}];
      if (opts.style === 'all_docs') {
        changeList = merge.collectLeaves(metadata.rev_tree)
        .map(function (x) { return {rev: x.rev}; });
      }
      var change = {
        id: metadata.id,
        changes: changeList,
        doc: doc
      };

      if (utils.isDeleted(metadata, doc._rev)) {
        change.deleted = true;
      }
      if (opts.conflicts) {
        change.doc._conflicts = merge.collectConflicts(metadata);
        if (!change.doc._conflicts.length) {
          delete change.doc._conflicts;
        }
      }
      return change;
    }
    api.changes = function (opts) {
      if (!api.taskqueue.ready()) {
        var task = api.taskqueue.addTask('changes', arguments);
        return {
          cancel: function () {
            if (task.task) {
              return task.task.cancel();
            }
            if (Pouch.DEBUG) {
              //console.log('Cancel Changes Feed');
            }
            task.parameters[0].aborted = true;
          }
        };
      }
      opts = utils.extend(true, {}, opts);
      opts.processChange = processChange;

      if (!opts.since) {
        opts.since = 0;
      }
      if (opts.since === 'latest') {
        var changes;
        api.info(function (err, info) {
          if (!opts.aborted) {
            opts.since = info.update_seq  - 1;
            api.changes(opts);
          }
        });
        // Return a method to cancel this method from processing any more
        return {
          cancel: function () {
            if (changes) {
              return changes.cancel();
            }
            if (Pouch.DEBUG) {
              //console.log('Cancel Changes Feed');
            }
            opts.aborted = true;
          }
        };
      }

      if (opts.filter && typeof opts.filter === 'string') {
        if (opts.filter === '_view') {
          if (opts.view && typeof opts.view === 'string') {
            // fetch a view from a design doc, make it behave like a filter
            var viewName = opts.view.split('/');
            api.get('_design/' + viewName[0], function (err, ddoc) {
              if (ddoc && ddoc.views && ddoc.views[viewName[1]]) {
                /*jshint evil: true */
                var filter = eval('(function () {' + 
                                  '  return function (doc) {' + 
                                  '    var emitted = false;' + 
                                  '    var emit = function (a, b) {' + 
                                  '      emitted = true;' + 
                                  '    };' + 
                                  '    var view = ' + ddoc.views[viewName[1]].map + ';' + 
                                  '    view(doc);' + 
                                  '    if (emitted) {' + 
                                  '      return true;' + 
                                  '    }' + 
                                  '  }' + 
                                  '})()');
                if (!opts.aborted) {
                  opts.filter = filter;
                  api.changes(opts);
                }
              } else {
                var msg = ddoc.views ? 'missing json key: ' + viewName[1] :
                  'missing json key: views';
                err = err || errors.error(errors.MISSING_DOC, msg);
                utils.call(opts.complete, err);
              }
            });
          } else {
            var err = errors.error(errors.BAD_REQUEST,
                                  '`view` filter parameter is not provided.');
            utils.call(opts.complete, err);
          }
        } else {
          // fetch a filter from a design doc
          var filterName = opts.filter.split('/');
          api.get('_design/' + filterName[0], function (err, ddoc) {
            if (ddoc && ddoc.filters && ddoc.filters[filterName[1]]) {
              /*jshint evil: true */
              var filter = eval('(function () { return ' +
                                ddoc.filters[filterName[1]] + ' })()');
              if (!opts.aborted) {
                opts.filter = filter;
                api.changes(opts);
              }
            } else {
              var msg = (ddoc && ddoc.filters) ? 'missing json key: ' + filterName[1]
                : 'missing json key: filters';
              err = err || errors.error(errors.MISSING_DOC, msg);
              utils.call(opts.complete, err);
            }
          });
        }
        // Return a method to cancel this method from processing any more
        return {
          cancel: function () {
            if (Pouch.DEBUG) {
              console.log('Cancel Changes Feed');
            }
            opts.aborted = true;
          }
        };
      }

      if (!('descending' in opts)) {
        opts.descending = false;
      }

      // 0 and 1 should return 1 document
      opts.limit = opts.limit === 0 ? 1 : opts.limit;
      return customApi._changes(opts);
    };

    api.close = function (callback) {
      if (!api.taskqueue.ready()) {
        api.taskqueue.addTask('close', arguments);
        return;
      }
      return customApi._close(callback);
    };

    api.info = function (callback) {
      if (!api.taskqueue.ready()) {
        api.taskqueue.addTask('info', arguments);
        return;
      }
      return customApi._info(callback);
    };

    api.id = function () {
      return customApi._id();
    };

    api.type = function () {
      return (typeof customApi._type === 'function') ? customApi._type() : opts.adapter;
    };

    api.bulkDocs = function (req, opts, callback) {
      if (!api.taskqueue.ready()) {
        api.taskqueue.addTask('bulkDocs', arguments);
        return;
      }
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      if (!opts) {
        opts = {};
      } else {
        opts = utils.extend(true, {}, opts);
      }

      if (!req || !req.docs || req.docs.length < 1) {
        return call(callback, errors.MISSING_BULK_DOCS);
      }

      if (!Array.isArray(req.docs)) {
        return call(callback, errors.QUERY_PARSE_ERROR);
      }

      for (var i = 0; i < req.docs.length; ++i) {
        if (typeof req.docs[i] !== 'object' || Array.isArray(req.docs[i])) {
          return call(callback, errors.NOT_AN_OBJECT);
        }
      }

      req = utils.extend(true, {}, req);
      if (!('new_edits' in opts)) {
        opts.new_edits = true;
      }

      return customApi._bulkDocs(req, opts, autoCompact(callback));
    };

    /* End Wrappers */
    var taskqueue = {};

    taskqueue.ready = false;
    taskqueue.queue = [];

    api.taskqueue = {};

    api.taskqueue.execute = function (db) {
      if (taskqueue.ready) {
        taskqueue.queue.forEach(function (d) {
          d.task = db[d.name].apply(null, d.parameters);
        });
      }
    };

    api.taskqueue.ready = function () {
      if (arguments.length === 0) {
        return taskqueue.ready;
      }
      taskqueue.ready = arguments[0];
    };

    api.taskqueue.addTask = function (name, parameters) {
      var task = { name: name, parameters: parameters };
      taskqueue.queue.push(task);
      return task;
    };

    api.replicate = {};

    api.replicate.from = function (url, opts, callback) {
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      return Pouch.replicate(url, customApi, opts, callback);
    };

    api.replicate.to = function (dbName, opts, callback) {
      if (typeof opts === 'function') {
        callback = opts;
        opts = {};
      }
      return Pouch.replicate(customApi, dbName, opts, callback);
    };

    for (var j in api) {
      if (!customApi.hasOwnProperty(j)) {
        customApi[j] = api[j];
      }
    }

    // Http adapter can skip setup so we force the db to be ready and execute any jobs
    if (opts.skipSetup) {
      api.taskqueue.ready(true);
      api.taskqueue.execute(api);
    }

    if (utils.isCordova()) {
      //to inform websql adapter that we can use api
      cordova.fireWindowEvent(opts.name + "_pouch", {});
    }
    return customApi;
  };
};

},{"./deps/errors":8,"./merge":13,"./utils":16}],2:[function(_dereq_,module,exports){
"use strict";

var utils = _dereq_('../utils');
var errors = _dereq_('../deps/errors');

// parseUri 1.2.2
// (c) Steven Levithan <stevenlevithan.com>
// MIT License
function parseUri(str) {
  var o = parseUri.options;
  var m = o.parser[o.strictMode ? "strict" : "loose"].exec(str);
  var uri = {};
  var i = 14;

  while (i--) {
    uri[o.key[i]] = m[i] || "";
  }

  uri[o.q.name] = {};
  uri[o.key[12]].replace(o.q.parser, function ($0, $1, $2) {
    if ($1) {
      uri[o.q.name][$1] = $2;
    }
  });

  return uri;
}

function encodeDocId(id) {
  if (/^_(design|local)/.test(id)) {
    return id;
  }
  return encodeURIComponent(id);
}

parseUri.options = {
  strictMode: false,
  key: ["source", "protocol", "authority", "userInfo", "user", "password", "host",
        "port", "relative", "path", "directory", "file", "query", "anchor"],
  q:   {
    name:   "queryKey",
    parser: /(?:^|&)([^&=]*)=?([^&]*)/g
  },
  parser: {
    strict: /^(?:([^:\/?#]+):)?(?:\/\/((?:(([^:@]*)(?::([^:@]*))?)?@)?([^:\/?#]*)(?::(\d*))?))?((((?:[^?#\/]*\/)*)([^?#]*))(?:\?([^#]*))?(?:#(.*))?)/,
    loose:  /^(?:(?![^:@]+:[^:@\/]*@)([^:\/?#.]+):)?(?:\/\/)?((?:(([^:@]*)(?::([^:@]*))?)?@)?([^:\/?#]*)(?::(\d*))?)(((\/(?:[^?#](?![^?#\/]*\.[^?#\/.]+(?:[?#]|$)))*\/?)?([^?#\/]*))(?:\?([^#]*))?(?:#(.*))?)/
  }
};

// Get all the information you possibly can about the URI given by name and
// return it as a suitable object.
function getHost(name, opts) {
  // If the given name contains "http:"
  if (/http(s?):/.test(name)) {
    // Prase the URI into all its little bits
    var uri = parseUri(name);

    // Store the fact that it is a remote URI
    uri.remote = true;

    // Store the user and password as a separate auth object
    if (uri.user || uri.password) {
      uri.auth = {username: uri.user, password: uri.password};
    }

    // Split the path part of the URI into parts using '/' as the delimiter
    // after removing any leading '/' and any trailing '/'
    var parts = uri.path.replace(/(^\/|\/$)/g, '').split('/');

    // Store the first part as the database name and remove it from the parts
    // array
    uri.db = parts.pop();

    // Restore the path by joining all the remaining parts (all the parts
    // except for the database name) with '/'s
    uri.path = parts.join('/');
    opts = opts || {};
    uri.headers = opts.headers || {};

    if (opts.auth || uri.auth) {
      var nAuth = opts.auth || uri.auth;
      var token = utils.btoa(nAuth.username + ':' + nAuth.password);
      uri.headers.Authorization = 'Basic ' + token;
    }

    if (opts.headers) {
      uri.headers = opts.headers;
    }

    return uri;
  }

  // If the given name does not contain 'http:' then return a very basic object
  // with no host, the current path, the given name as the database name and no
  // username/password
  return {host: '', path: '/', db: name, auth: false};
}

// Generate a URL with the host data given by opts and the given path
function genDBUrl(opts, path) {
  // If the host is remote
  if (opts.remote) {
    // If the host already has a path, then we need to have a path delimiter
    // Otherwise, the path delimiter is the empty string
    var pathDel = !opts.path ? '' : '/';

    // Return the URL made up of all the host's information and the given path
    return opts.protocol + '://' + opts.host + ':' + opts.port + '/' +
      opts.path + pathDel + opts.db + '/' + path;
  }

  // If the host is not remote, then return the URL made up of just the
  // database name and the given path
  return '/' + opts.db + '/' + path;
}

// Generate a URL with the host data given by opts and the given path
function genUrl(opts, path) {
  if (opts.remote) {
    // If the host already has a path, then we need to have a path delimiter
    // Otherwise, the path delimiter is the empty string
    var pathDel = !opts.path ? '' : '/';

    // If the host already has a path, then we need to have a path delimiter
    // Otherwise, the path delimiter is the empty string
    return opts.protocol + '://' + opts.host + ':' + opts.port + '/' + opts.path + pathDel + path;
  }

  return '/' + path;
}

// Implements the PouchDB API for dealing with CouchDB instances over HTTP
function HttpPouch(opts, callback) {

  // Parse the URI given by opts.name into an easy-to-use object
  var host = getHost(opts.name, opts);

  // Generate the database URL based on the host
  var db_url = genDBUrl(host, '');

  // The functions that will be publically available for HttpPouch
  var api = {};
  var ajaxOpts = opts.ajax || {};
  function ajax(options, callback) {
    return utils.ajax(utils.extend({}, ajaxOpts, options), callback);
  }
  var uuids = {
    list: [],
    get: function (opts, callback) {
      if (typeof opts === 'function') {
        callback = opts;
        opts = {count: 10};
      }
      var cb = function (err, body) {
        if (err || !('uuids' in body)) {
          utils.call(callback, err || errors.UNKNOWN_ERROR);
        } else {
          uuids.list = uuids.list.concat(body.uuids);
          utils.call(callback, null, "OK");
        }
      };
      var params = '?count=' + opts.count;
      ajax({
        headers: host.headers,
        method: 'GET',
        url: genUrl(host, '_uuids') + params
      }, cb);
    }
  };

  // Create a new CouchDB database based on the given opts
  var createDB = function () {
    ajax({headers: host.headers, method: 'PUT', url: db_url}, function (err, ret) {
      // If we get an "Unauthorized" error
      if (err && err.status === 401) {
        // Test if the database already exists
        ajax({headers: host.headers, method: 'HEAD', url: db_url}, function (err, ret) {
          // If there is still an error
          if (err) {
            // Give the error to the callback to deal with
            utils.call(callback, err);
          } else {
            // Continue as if there had been no errors
            utils.call(callback, null, api);
          }
        });
        // If there were no errros or if the only error is "Precondition Failed"
        // (note: "Precondition Failed" occurs when we try to create a database
        // that already exists)
      } else if (!err || err.status === 412) {
        // Continue as if there had been no errors
        utils.call(callback, null, api);
      } else {
        utils.call(callback, errors.UNKNOWN_ERROR);
      }
    });
  };
  if (!opts.skipSetup) {
    ajax({headers: host.headers, method: 'GET', url: db_url}, function (err, ret) {
      //check if the db exists
      if (err) {
        if (err.status === 404) {
          //if it doesn't, create it
          createDB();
        } else {
          utils.call(callback, err);
        }
      } else {
        //go do stuff with the db
        utils.call(callback, null, api);
      }
    });
  }

  api.type = function () {
    return 'http';
  };

  // The HttpPouch's ID is its URL
  api.id = function () {
    return genDBUrl(host, '');
  };

  api.request = function (options, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('request', arguments);
      return;
    }
    options.headers = host.headers;
    options.url = genDBUrl(host, options.url);
    ajax(options, callback);
  };

  // Sends a POST request to the host calling the couchdb _compact function
  //    version: The version of CouchDB it is running
  api.compact = function (opts, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('compact', arguments);
      return;
    }
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }
    ajax({
      headers: host.headers,
      url: genDBUrl(host, '_compact'),
      method: 'POST'
    }, function () {
      function ping() {
        api.info(function (err, res) {
          if (!res.compact_running) {
            utils.call(callback, null);
          } else {
            setTimeout(ping, opts.interval || 200);
          }
        });
      }
      // Ping the http if it's finished compaction
      if (typeof callback === "function") {
        ping();
      }
    });
  };

  // Calls GET on the host, which gets back a JSON string containing
  //    couchdb: A welcome string
  //    version: The version of CouchDB it is running
  api.info = function (callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('info', arguments);
      return;
    }
    ajax({
      headers: host.headers,
      method: 'GET',
      url: genDBUrl(host, '')
    }, callback);
  };

  // Get the document with the given id from the database given by host.
  // The id could be solely the _id in the database, or it may be a
  // _design/ID or _local/ID path
  api.get = function (id, opts, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('get', arguments);
      return;
    }
    // If no options were given, set the callback to the second parameter
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }

    if (opts.auto_encode === undefined) {
      opts.auto_encode = true;
    }

    // List of parameters to add to the GET request
    var params = [];

    // If it exists, add the opts.revs value to the list of parameters.
    // If revs=true then the resulting JSON will include a field
    // _revisions containing an array of the revision IDs.
    if (opts.revs) {
      params.push('revs=true');
    }

    // If it exists, add the opts.revs_info value to the list of parameters.
    // If revs_info=true then the resulting JSON will include the field
    // _revs_info containing an array of objects in which each object
    // representing an available revision.
    if (opts.revs_info) {
      params.push('revs_info=true');
    }

    if (opts.local_seq) {
      params.push('local_seq=true');
    }
    // If it exists, add the opts.open_revs value to the list of parameters.
    // If open_revs=all then the resulting JSON will include all the leaf
    // revisions. If open_revs=["rev1", "rev2",...] then the resulting JSON
    // will contain an array of objects containing data of all revisions
    if (opts.open_revs) {
      if (opts.open_revs !== "all") {
        opts.open_revs = JSON.stringify(opts.open_revs);
      }
      params.push('open_revs=' + opts.open_revs);
    }

    // If it exists, add the opts.attachments value to the list of parameters.
    // If attachments=true the resulting JSON will include the base64-encoded
    // contents in the "data" property of each attachment.
    if (opts.attachments) {
      params.push('attachments=true');
    }

    // If it exists, add the opts.rev value to the list of parameters.
    // If rev is given a revision number then get the specified revision.
    if (opts.rev) {
      params.push('rev=' + opts.rev);
    }

    // If it exists, add the opts.conflicts value to the list of parameters.
    // If conflicts=true then the resulting JSON will include the field
    // _conflicts containing all the conflicting revisions.
    if (opts.conflicts) {
      params.push('conflicts=' + opts.conflicts);
    }

    // Format the list of parameters into a valid URI query string
    params = params.join('&');
    params = params === '' ? '' : '?' + params;

    if (opts.auto_encode) {
      id = encodeDocId(id);
    }

    // Set the options for the ajax call
    var options = {
      headers: host.headers,
      method: 'GET',
      url: genDBUrl(host, id + params)
    };

    // If the given id contains at least one '/' and the part before the '/'
    // is NOT "_design" and is NOT "_local"
    // OR
    // If the given id contains at least two '/' and the part before the first
    // '/' is "_design".
    // TODO This second condition seems strange since if parts[0] === '_design'
    // then we already know that parts[0] !== '_local'.
    var parts = id.split('/');
    if ((parts.length > 1 && parts[0] !== '_design' && parts[0] !== '_local') ||
        (parts.length > 2 && parts[0] === '_design' && parts[0] !== '_local')) {
      // Binary is expected back from the server
      options.binary = true;
    }

    // Get the document
    ajax(options, function (err, doc, xhr) {
      // If the document does not exist, send an error to the callback
      if (err) {
        return utils.call(callback, err);
      }

      // Send the document to the callback
      utils.call(callback, null, doc, xhr);
    });
  };

  // Delete the document given by doc from the database given by host.
  api.remove = function (doc, opts, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('remove', arguments);
      return;
    }
    // If no options were given, set the callback to be the second parameter
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }

    // Delete the document
    ajax({
      headers: host.headers,
      method: 'DELETE',
      url: genDBUrl(host, encodeDocId(doc._id)) + '?rev=' + doc._rev
    }, callback);
  };

  // Get the attachment
  api.getAttachment = function (docId, attachmentId, opts, callback) {
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }
    if (opts.auto_encode === undefined) {
      opts.auto_encode = true;
    }
    if (opts.auto_encode) {
      docId = encodeDocId(docId);
    }
    opts.auto_encode = false;
    api.get(docId + '/' + attachmentId, opts, callback);
  };

  // Remove the attachment given by the id and rev
  api.removeAttachment = function (docId, attachmentId, rev, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('removeAttachment', arguments);
      return;
    }
    ajax({
      headers: host.headers,
      method: 'DELETE',
      url: genDBUrl(host, encodeDocId(docId) + '/' + attachmentId) + '?rev=' + rev
    }, callback);
  };

  // Add the attachment given by blob and its contentType property
  // to the document with the given id, the revision given by rev, and
  // add it to the database given by host.
  api.putAttachment = function (docId, attachmentId, rev, blob, type, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('putAttachment', arguments);
      return;
    }
    if (typeof type === 'function') {
      callback = type;
      type = blob;
      blob = rev;
      rev = null;
    }
    if (typeof type === 'undefined') {
      type = blob;
      blob = rev;
      rev = null;
    }
    var id = encodeDocId(docId) + '/' + attachmentId;
    var url = genDBUrl(host, id);
    if (rev) {
      url += '?rev=' + rev;
    }

    var opts = {
      headers: host.headers,
      method: 'PUT',
      url: url,
      processData: false,
      body: blob,
      timeout: 60000
    };
    opts.headers['Content-Type'] = type;
    // Add the attachment
    ajax(opts, callback);
  };

  // Add the document given by doc (in JSON string format) to the database
  // given by host. This fails if the doc has no _id field.
  api.put = function (doc, opts, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('put', arguments);
      return;
    }
    // If no options were given, set the callback to be the second parameter
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }
    if (typeof doc !== 'object') {
      return utils.call(callback, errors.NOT_AN_OBJECT);
    }
    if (!utils.isValidId(doc._id)) {
      return utils.call(callback, errors.MISSING_ID);
    }

    // List of parameter to add to the PUT request
    var params = [];

    // If it exists, add the opts.new_edits value to the list of parameters.
    // If new_edits = false then the database will NOT assign this document a
    // new revision number
    if (opts && typeof opts.new_edits !== 'undefined') {
      params.push('new_edits=' + opts.new_edits);
    }

    // Format the list of parameters into a valid URI query string
    params = params.join('&');
    if (params !== '') {
      params = '?' + params;
    }

    // Add the document
    ajax({
      headers: host.headers,
      method: 'PUT',
      url: genDBUrl(host, encodeDocId(doc._id)) + params,
      body: doc
    }, callback);
  };

  // Add the document given by doc (in JSON string format) to the database
  // given by host. This does not assume that doc is a new document (i.e. does not
  // have a _id or a _rev field.
  api.post = function (doc, opts, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('post', arguments);
      return;
    }
    // If no options were given, set the callback to be the second parameter
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }
    if (typeof doc !== 'object') {
      return utils.call(callback, errors.NOT_AN_OBJECT);
    }
    if (! ("_id" in doc)) {
      if (uuids.list.length > 0) {
        doc._id = uuids.list.pop();
        api.put(doc, opts, callback);
      } else {
        uuids.get(function (err, resp) {
          if (err) {
            return utils.call(callback, errors.UNKNOWN_ERROR);
          }
          doc._id = uuids.list.pop();
          api.put(doc, opts, callback);
        });
      }
    } else {
      api.put(doc, opts, callback);
    }
  };

  // Update/create multiple documents given by req in the database
  // given by host.
  api.bulkDocs = function (req, opts, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('bulkDocs', arguments);
      return;
    }
    // If no options were given, set the callback to be the second parameter
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }
    if (!opts) {
      opts = {};
    }

    // If opts.new_edits exists add it to the document data to be
    // send to the database.
    // If new_edits=false then it prevents the database from creating
    // new revision numbers for the documents. Instead it just uses
    // the old ones. This is used in database replication.
    if (typeof opts.new_edits !== 'undefined') {
      req.new_edits = opts.new_edits;
    }

    // Update/create the documents
    ajax({
      headers: host.headers,
      method: 'POST',
      url: genDBUrl(host, '_bulk_docs'),
      body: req
    }, callback);
  };

  // Get a listing of the documents in the database given
  // by host and ordered by increasing id.
  api.allDocs = function (opts, callback) {
    // If no options were given, set the callback to be the second parameter
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('allDocs', arguments);
      return;
    }
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }

    // List of parameters to add to the GET request
    var params = [];
    var body;
    var method = 'GET';

    // TODO I don't see conflicts as a valid parameter for a
    // _all_docs request (see http://wiki.apache.org/couchdb/HTTP_Document_API#all_docs)
    if (opts.conflicts) {
      params.push('conflicts=true');
    }

    // If opts.descending is truthy add it to params
    if (opts.descending) {
      params.push('descending=true');
    }

    // If opts.include_docs exists, add the include_docs value to the
    // list of parameters.
    // If include_docs=true then include the associated document with each
    // result.
    if (opts.include_docs) {
      params.push('include_docs=true');
    }

    // If opts.startkey exists, add the startkey value to the list of
    // parameters.
    // If startkey is given then the returned list of documents will
    // start with the document whose id is startkey.
    if (opts.startkey) {
      params.push('startkey=' +
                  encodeURIComponent(JSON.stringify(opts.startkey)));
    }

    // If opts.endkey exists, add the endkey value to the list of parameters.
    // If endkey is given then the returned list of docuemnts will
    // end with the document whose id is endkey.
    if (opts.endkey) {
      params.push('endkey=' + encodeURIComponent(JSON.stringify(opts.endkey)));
    }

    // If opts.limit exists, add the limit value to the parameter list.
    if (opts.limit) {
      params.push('limit=' + opts.limit);
    }

    if (typeof opts.skip !== 'undefined') {
      params.push('skip=' + opts.skip);
    }

    // Format the list of parameters into a valid URI query string
    params = params.join('&');
    if (params !== '') {
      params = '?' + params;
    }

    // If keys are supplied, issue a POST request to circumvent GET query string limits
    // see http://wiki.apache.org/couchdb/HTTP_view_API#Querying_Options
    if (typeof opts.keys !== 'undefined') {
      method = 'POST';
      body = JSON.stringify({keys: opts.keys});
    }

    // Get the document listing
    ajax({
      headers: host.headers,
      method: method,
      url: genDBUrl(host, '_all_docs' + params),
      body: body
    }, callback);
  };

  // Get a list of changes made to documents in the database given by host.
  // TODO According to the README, there should be two other methods here,
  // api.changes.addListener and api.changes.removeListener.
  api.changes = function (opts) {

    // We internally page the results of a changes request, this means
    // if there is a large set of changes to be returned we can start
    // processing them quicker instead of waiting on the entire
    // set of changes to return and attempting to process them at once
    var CHANGES_LIMIT = 25;

    if (!api.taskqueue.ready()) {
      var task = api.taskqueue.addTask('changes', arguments);
      return {
        cancel: function () {
          if (task.task) {
            return task.task.cancel();
          }
          //console.log(db_url + ': Cancel Changes Feed');
          task.parameters[0].aborted = true;
        }
      };
    }

    if (opts.since === 'latest') {
      var changes;
      api.info(function (err, info) {
        if (!opts.aborted) {
          opts.since = info.update_seq;
          changes = api.changes(opts);
        }
      });
      // Return a method to cancel this method from processing any more
      return {
        cancel: function () {
          if (changes) {
            return changes.cancel();
          }
          //console.log(db_url + ': Cancel Changes Feed');
          opts.aborted = true;
        }
      };
    }

    //console.log(db_url + ': Start Changes Feed: continuous=' + opts.continuous);

    var params = {};
    var limit = (typeof opts.limit !== 'undefined') ? opts.limit : false;
    if (limit === 0) {
      limit = 1;
    }
    //
    var leftToFetch = limit;

    if (opts.style) {
      params.style = opts.style;
    }

    if (opts.include_docs || opts.filter && typeof opts.filter === 'function') {
      params.include_docs = true;
    }

    if (opts.continuous) {
      params.feed = 'longpoll';
    }

    if (opts.conflicts) {
      params.conflicts = true;
    }

    if (opts.descending) {
      params.descending = true;
    }

    if (opts.filter && typeof opts.filter === 'string') {
      params.filter = opts.filter;
      if (opts.filter === '_view' && opts.view && typeof opts.view === 'string') {
        params.view = opts.view;
      }
    }

    // If opts.query_params exists, pass it through to the changes request.
    // These parameters may be used by the filter on the source database.
    if (opts.query_params && typeof opts.query_params === 'object') {
      for (var param_name in opts.query_params) {
        if (opts.query_params.hasOwnProperty(param_name)) {
          params[param_name] = opts.query_params[param_name];
        }
      }
    }

    var xhr;
    var lastFetchedSeq;
    var remoteLastSeq;
    var pagingCount;

    // Get all the changes starting wtih the one immediately after the
    // sequence number given by since.
    var fetch = function (since, callback) {
      params.since = since;
      if (!opts.continuous && !pagingCount) {
        pagingCount = remoteLastSeq;
      }
      params.limit = (!limit || leftToFetch > CHANGES_LIMIT) ?
        CHANGES_LIMIT : leftToFetch;

      var paramStr = '?' + Object.keys(params).map(function (k) {
        return k + '=' + params[k];
      }).join('&');

      // Set the options for the ajax call
      var xhrOpts = {
        headers: host.headers,
        method: 'GET',
        url: genDBUrl(host, '_changes' + paramStr),
        // _changes can take a long time to generate, especially when filtered
        timeout: null
      };
      lastFetchedSeq = since;

      if (opts.aborted) {
        return;
      }

      // Get the changes
      xhr = ajax(xhrOpts, callback);
    };

    // If opts.since exists, get all the changes from the sequence
    // number given by opts.since. Otherwise, get all the changes
    // from the sequence number 0.
    var fetchTimeout = 10;
    var fetchRetryCount = 0;

    var results = {results: []};

    var fetched = function (err, res) {
      // If the result of the ajax call (res) contains changes (res.results)
      if (res && res.results) {
        results.last_seq = res.last_seq;
        // For each change
        var req = {};
        req.query = opts.query_params;
        res.results = res.results.filter(function (c) {
          leftToFetch--;
          var ret = utils.filterChange(opts)(c);
          if (ret) {
            results.results.push(c);
            utils.call(opts.onChange, c);
          }
          return ret;
        });
      } else if (err) {
        // In case of an error, stop listening for changes and call opts.complete
        opts.aborted = true;
        utils.call(opts.complete, err, null);
      }

      // The changes feed may have timed out with no results
      // if so reuse last update sequence
      if (res && res.last_seq) {
        lastFetchedSeq = res.last_seq;
      }

      var resultsLength = res && res.results.length || 0;

      pagingCount -= CHANGES_LIMIT;

      var finished = (limit && leftToFetch <= 0) ||
        (res && !resultsLength && pagingCount <= 0) ||
        (resultsLength && res.last_seq === remoteLastSeq) ||
        (opts.descending && lastFetchedSeq !== 0);

      if (opts.continuous || !finished) {
        // Increase retry delay exponentially as long as errors persist
        if (err) {
          fetchRetryCount += 1;
        } else {
          fetchRetryCount = 0;
        }
        var timeoutMultiplier = 1 << fetchRetryCount;
        var retryWait = fetchTimeout * timeoutMultiplier;
        var maximumWait = opts.maximumWait || 30000;

        if (retryWait > maximumWait) {
          utils.call(opts.complete, err || errors.UNKNOWN_ERROR, null);
        }

        // Queue a call to fetch again with the newest sequence number
        setTimeout(function () { fetch(lastFetchedSeq, fetched); }, retryWait);
      } else {
        // We're done, call the callback
        utils.call(opts.complete, null, results);
      }
    };

    // If we arent doing a continuous changes request we need to know
    // the current update_seq so we know when to stop processing the
    // changes
    if (opts.continuous) {
      fetch(opts.since || 0, fetched);
    } else {
      api.info(function (err, res) {
        if (err) {
          return utils.call(opts.complete, err);
        }
        remoteLastSeq = res.update_seq;
        fetch(opts.since || 0, fetched);
      });
    }

    // Return a method to cancel this method from processing any more
    return {
      cancel: function () {
        //console.log(db_url + ': Cancel Changes Feed');
        opts.aborted = true;
        xhr.abort();
      }
    };
  };

  // Given a set of document/revision IDs (given by req), tets the subset of
  // those that do NOT correspond to revisions stored in the database.
  // See http://wiki.apache.org/couchdb/HttpPostRevsDiff
  api.revsDiff = function (req, opts, callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('revsDiff', arguments);
      return;
    }
    // If no options were given, set the callback to be the second parameter
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }

    // Get the missing document/revision IDs
    ajax({
      headers: host.headers,
      method: 'POST',
      url: genDBUrl(host, '_revs_diff'),
      body: req
    }, function (err, res) {
      utils.call(callback, err, res);
    });
  };

  api.close = function (callback) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('close', arguments);
      return;
    }
    utils.call(callback, null);
  };

  api.replicateOnServer = function (target, opts, promise) {
    if (!api.taskqueue.ready()) {
      api.taskqueue.addTask('replicateOnServer', arguments);
      return promise;
    }

    var targetHost = getHost(target.id());
    var params = {
      source: host.db,
      target: targetHost.protocol === host.protocol && targetHost.authority === host.authority ? targetHost.db : targetHost.source
    };

    if (opts.continuous) {
      params.continuous = true;
    }

    if (opts.create_target) {
      params.create_target = true;
    }

    if (opts.doc_ids) {
      params.doc_ids = opts.doc_ids;
    }

    if (opts.filter && typeof opts.filter === 'string') {
      params.filter = opts.filter;
    }

    if (opts.query_params) {
      params.query_params = opts.query_params;
    }

    var result = {};
    var repOpts = {
      headers: host.headers,
      method: 'POST',
      url: host.protocol + '://' + host.host + (host.port === 80 ? '' : (':' + host.port)) + '/_replicate',
      body: params
    };
    var xhr;
    promise.cancel = function () {
      this.cancelled = true;
      if (xhr && !result.ok) {
        xhr.abort();
      }
      if (result._local_id) {
        repOpts.body = {
          replication_id: result._local_id
        };
      }
      repOpts.body.cancel = true;
      ajax(repOpts, function (err, resp, xhr) {
        // If the replication cancel request fails, send an error to the callback
        if (err) {
          return utils.call(callback, err);
        }
        // Send the replication cancel result to the complete callback
        utils.call(opts.complete, null, result, xhr);
      });
    };

    if (promise.cancelled) {
      return;
    }

    xhr = ajax(repOpts, function (err, resp, xhr) {
      // If the replication fails, send an error to the callback
      if (err) {
        return utils.call(callback, err);
      }

      result.ok = true;

      // Provided by CouchDB from 1.2.0 onward to cancel replication
      if (resp._local_id) {
        result._local_id = resp._local_id;
      }

      // Send the replication result to the complete callback
      utils.call(opts.complete, null, resp, xhr);
    });
  };

  return api;
}

// Delete the HttpPouch specified by the given name.
HttpPouch.destroy = function (name, opts, callback) {
  var host = getHost(name, opts);
  opts = opts || {};
  if (typeof opts === 'function') {
    callback = opts;
    opts = {};
  }
  opts.headers = host.headers;
  opts.method = 'DELETE';
  opts.url = genDBUrl(host, '');
  utils.ajax(opts, callback);
};

// HttpPouch is a valid adapter.
HttpPouch.valid = function () {
  return true;
};

module.exports = HttpPouch;

},{"../deps/errors":8,"../utils":16}],3:[function(_dereq_,module,exports){
'use strict';

var utils = _dereq_('../utils');
var merge = _dereq_('../merge');
var errors = _dereq_('../deps/errors');
function idbError(callback) {
  return function (event) {
    utils.call(callback, errors.error(errors.IDB_ERROR, event.target, event.type));
  };
}

function IdbPouch(opts, callback) {

  // IndexedDB requires a versioned database structure, this is going to make
  // it hard to dynamically create object stores if we needed to for things
  // like views
  var POUCH_VERSION = 1;

  // The object stores created for each database
  // DOC_STORE stores the document meta data, its revision history and state
  var DOC_STORE = 'document-store';
  // BY_SEQ_STORE stores a particular version of a document, keyed by its
  // sequence id
  var BY_SEQ_STORE = 'by-sequence';
  // Where we store attachments
  var ATTACH_STORE = 'attach-store';
  // Where we store meta data
  var META_STORE = 'meta-store';
  // Where we detect blob support
  var DETECT_BLOB_SUPPORT_STORE = 'detect-blob-support';


  var name = opts.name;
  var req = window.indexedDB.open(name, POUCH_VERSION);

  if (!('openReqList' in IdbPouch)) {
    IdbPouch.openReqList = {};
  }
  IdbPouch.openReqList[name] = req;

  var blobSupport = null;

  var instanceId = null;
  var api = {};
  var idb = null;

  req.onupgradeneeded = function (e) {
    var db = e.target.result;
    var currentVersion = e.oldVersion;
    while (currentVersion !== e.newVersion) {
      if (currentVersion === 0) {
        createSchema(db);
      }
      currentVersion++;
    }
  };

  function createSchema(db) {
    db.createObjectStore(DOC_STORE, {keyPath : 'id'})
      .createIndex('seq', 'seq', {unique: true});
    db.createObjectStore(BY_SEQ_STORE, {autoIncrement : true})
      .createIndex('_doc_id_rev', '_doc_id_rev', {unique: true});
    db.createObjectStore(ATTACH_STORE, {keyPath: 'digest'});
    db.createObjectStore(META_STORE, {keyPath: 'id', autoIncrement: false});
    db.createObjectStore(DETECT_BLOB_SUPPORT_STORE);
  }

  // From http://stackoverflow.com/questions/14967647/encode-decode-image-with-base64-breaks-image (2013-04-21)
  function fixBinary(bin) {
    var length = bin.length;
    var buf = new ArrayBuffer(length);
    var arr = new Uint8Array(buf);
    for (var i = 0; i < length; i++) {
      arr[i] = bin.charCodeAt(i);
    }
    return buf;
  }

  req.onsuccess = function (e) {

    idb = e.target.result;

    idb.onversionchange = function () {
      idb.close();
    };

    var txn = idb.transaction([META_STORE, DETECT_BLOB_SUPPORT_STORE],
                              'readwrite');

    var req = txn.objectStore(META_STORE).get(META_STORE);

    req.onsuccess = function (e) {
      var meta = e.target.result || {id: META_STORE};
      if (name + '_id' in meta) {
        instanceId = meta[name + '_id'];
      } else {
        instanceId = utils.uuid();
        meta[name + '_id'] = instanceId;
        txn.objectStore(META_STORE).put(meta);
      }

      // detect blob support
      try {
        txn.objectStore(DETECT_BLOB_SUPPORT_STORE).put(utils.createBlob(), "key");
        blobSupport = true;
      } catch (err) {
        blobSupport = false;
      } finally {
        utils.call(callback, null, api);
      }
    };
  };

  req.onerror = idbError(callback);

  api.type = function () {
    return 'idb';
  };

  // Each database needs a unique id so that we can store the sequence
  // checkpoint without having other databases confuse itself.
  api.id = function idb_id() {
    return instanceId;
  };

  api._bulkDocs = function idb_bulkDocs(req, opts, callback) {
    var newEdits = opts.new_edits;
    var userDocs = req.docs;
    // Parse the docs, give them a sequence number for the result
    var docInfos = userDocs.map(function (doc, i) {
      var newDoc = utils.parseDoc(doc, newEdits);
      newDoc._bulk_seq = i;
      return newDoc;
    });

    var docInfoErrors = docInfos.filter(function (docInfo) {
      return docInfo.error;
    });
    if (docInfoErrors.length) {
      return utils.call(callback, docInfoErrors[0]);
    }

    var results = [];
    var docsWritten = 0;

    function writeMetaData(e) {
      var meta = e.target.result;
      meta.updateSeq = (meta.updateSeq || 0) + docsWritten;
      txn.objectStore(META_STORE).put(meta);
    }

    function processDocs() {
      if (!docInfos.length) {
        txn.objectStore(META_STORE).get(META_STORE).onsuccess = writeMetaData;
        return;
      }
      var currentDoc = docInfos.shift();
      var req = txn.objectStore(DOC_STORE).get(currentDoc.metadata.id);
      req.onsuccess = function process_docRead(event) {
        var oldDoc = event.target.result;
        if (!oldDoc) {
          insertDoc(currentDoc);
        } else {
          updateDoc(oldDoc, currentDoc);
        }
      };
    }

    function complete(event) {
      var aresults = [];
      results.sort(sortByBulkSeq);
      results.forEach(function (result) {
        delete result._bulk_seq;
        if (result.error) {
          aresults.push(result);
          return;
        }
        var metadata = result.metadata;
        var rev = merge.winningRev(metadata);

        aresults.push({
          ok: true,
          id: metadata.id,
          rev: rev
        });

        if (utils.isLocalId(metadata.id)) {
          return;
        }

        IdbPouch.Changes.notify(name);
        IdbPouch.Changes.notifyLocalWindows(name);
      });
      utils.call(callback, null, aresults);
    }

    function preprocessAttachment(att, finish) {
      if (att.stub) {
        return finish();
      }
      if (typeof att.data === 'string') {
        var data;
        try {
          data = atob(att.data);
        } catch (e) {
          var err = errors.error(errors.BAD_ARG,
                                "Attachments need to be base64 encoded");
          return utils.call(callback, err);
        }
        att.digest = 'md5-' + utils.Crypto.MD5(data);
        if (blobSupport) {
          var type = att.content_type;
          data = fixBinary(data);
          att.data = utils.createBlob([data], {type: type});
        }
        return finish();
      }
      var reader = new FileReader();
      reader.onloadend = function (e) {
        att.digest = 'md5-' + utils.Crypto.MD5(this.result);
        if (!blobSupport) {
          att.data = btoa(this.result);
        }
        finish();
      };
      reader.readAsBinaryString(att.data);
    }

    function preprocessAttachments(callback) {
      if (!docInfos.length) {
        return callback();
      }

      var docv = 0;
      docInfos.forEach(function (docInfo) {
        var attachments = docInfo.data && docInfo.data._attachments ?
          Object.keys(docInfo.data._attachments) : [];

        if (!attachments.length) {
          return done();
        }

        var recv = 0;
        function attachmentProcessed() {
          recv++;
          if (recv === attachments.length) {
            done();
          }
        }

        for (var key in docInfo.data._attachments) {
          preprocessAttachment(docInfo.data._attachments[key], attachmentProcessed);
        }
      });

      function done() {
        docv++;
        if (docInfos.length === docv) {
          callback();
        }
      }
    }

    function writeDoc(docInfo, callback) {
      var err = null;
      var recv = 0;
      docInfo.data._id = docInfo.metadata.id;
      docInfo.data._rev = docInfo.metadata.rev;

      docsWritten++;

      if (utils.isDeleted(docInfo.metadata, docInfo.metadata.rev)) {
        docInfo.data._deleted = true;
      }

      var attachments = docInfo.data._attachments ?
        Object.keys(docInfo.data._attachments) : [];

      function collectResults(attachmentErr) {
        if (!err) {
          if (attachmentErr) {
            err = attachmentErr;
            utils.call(callback, err);
          } else if (recv === attachments.length) {
            finish();
          }
        }
      }

      function attachmentSaved(err) {
        recv++;
        collectResults(err);
      }

      for (var key in docInfo.data._attachments) {
        if (!docInfo.data._attachments[key].stub) {
          var data = docInfo.data._attachments[key].data;
          delete docInfo.data._attachments[key].data;
          var digest = docInfo.data._attachments[key].digest;
          saveAttachment(docInfo, digest, data, attachmentSaved);
        } else {
          recv++;
          collectResults();
        }
      }

      function finish() {
        docInfo.data._doc_id_rev = docInfo.data._id + "::" + docInfo.data._rev;
        var dataReq = txn.objectStore(BY_SEQ_STORE).put(docInfo.data);
        dataReq.onsuccess = function (e) {
          //console.log(name + ': Wrote Document ', docInfo.metadata.id);
          docInfo.metadata.seq = e.target.result;
          // Current _rev is calculated from _rev_tree on read
          delete docInfo.metadata.rev;
          var metaDataReq = txn.objectStore(DOC_STORE).put(docInfo.metadata);
          metaDataReq.onsuccess = function () {
            results.push(docInfo);
            utils.call(callback);
          };
        };
      }

      if (!attachments.length) {
        finish();
      }
    }

    function updateDoc(oldDoc, docInfo) {
      var merged = merge.merge(oldDoc.rev_tree, docInfo.metadata.rev_tree[0], 1000);
      var wasPreviouslyDeleted = utils.isDeleted(oldDoc);
      var inConflict = (wasPreviouslyDeleted &&
                        utils.isDeleted(docInfo.metadata)) ||
        (!wasPreviouslyDeleted && newEdits && merged.conflicts !== 'new_leaf');

      if (inConflict) {
        results.push(makeErr(errors.REV_CONFLICT, docInfo._bulk_seq));
        return processDocs();
      }

      docInfo.metadata.rev_tree = merged.tree;
      writeDoc(docInfo, processDocs);
    }

    function insertDoc(docInfo) {
      // Cant insert new deleted documents
      if ('was_delete' in opts && utils.isDeleted(docInfo.metadata)) {
        results.push(errors.MISSING_DOC);
        return processDocs();
      }
      writeDoc(docInfo, processDocs);
    }

    // Insert sequence number into the error so we can sort later
    function makeErr(err, seq) {
      err._bulk_seq = seq;
      return err;
    }

    function saveAttachment(docInfo, digest, data, callback) {
      var objectStore = txn.objectStore(ATTACH_STORE);
      var getReq = objectStore.get(digest).onsuccess = function (e) {
        var originalRefs = e.target.result && e.target.result.refs || {};
        var ref = [docInfo.metadata.id, docInfo.metadata.rev].join('@');
        var newAtt = {
          digest: digest,
          body: data,
          refs: originalRefs
        };
        newAtt.refs[ref] = true;
        var putReq = objectStore.put(newAtt).onsuccess = function (e) {
          utils.call(callback);
        };
      };
    }

    var txn;
    preprocessAttachments(function () {
      txn = idb.transaction([DOC_STORE, BY_SEQ_STORE, ATTACH_STORE, META_STORE],
                            'readwrite');
      txn.onerror = idbError(callback);
      txn.ontimeout = idbError(callback);
      txn.oncomplete = complete;

      processDocs();
    });
  };

  function sortByBulkSeq(a, b) {
    return a._bulk_seq - b._bulk_seq;
  }

  // First we look up the metadata in the ids database, then we fetch the
  // current revision(s) from the by sequence store
  api._get = function idb_get(id, opts, callback) {
    var doc;
    var metadata;
    var err;
    var txn;
    if (opts.ctx) {
      txn = opts.ctx;
    } else {
      txn = idb.transaction([DOC_STORE, BY_SEQ_STORE, ATTACH_STORE], 'readonly');
    }

    function finish() {
      utils.call(callback, err, {doc: doc, metadata: metadata, ctx: txn});
    }

    txn.objectStore(DOC_STORE).get(id).onsuccess = function (e) {
      metadata = e.target.result;
      // we can determine the result here if:
      // 1. there is no such document
      // 2. the document is deleted and we don't ask about specific rev
      // When we ask with opts.rev we expect the answer to be either
      // doc (possibly with _deleted=true) or missing error
      if (!metadata) {
        err = errors.MISSING_DOC;
        return finish();
      }
      if (utils.isDeleted(metadata) && !opts.rev) {
        err = errors.error(errors.MISSING_DOC, "deleted");
        return finish();
      }

      var rev = merge.winningRev(metadata);
      var key = metadata.id + '::' + (opts.rev ? opts.rev : rev);
      var index = txn.objectStore(BY_SEQ_STORE).index('_doc_id_rev');

      index.get(key).onsuccess = function (e) {
        doc = e.target.result;
        if (doc && doc._doc_id_rev) {
          delete(doc._doc_id_rev);
        }
        if (!doc) {
          err = errors.MISSING_DOC;
          return finish();
        }
        finish();
      };
    };
  };

  api._getAttachment = function (attachment, opts, callback) {
    var result;
    var txn;
    if (opts.ctx) {
      txn = opts.ctx;
    } else {
      txn = idb.transaction([DOC_STORE, BY_SEQ_STORE, ATTACH_STORE], 'readonly');
    }
    var digest = attachment.digest;
    var type = attachment.content_type;

    txn.objectStore(ATTACH_STORE).get(digest).onsuccess = function (e) {
      var data = e.target.result.body;
      if (opts.encode) {
        if (blobSupport) {
          var reader = new FileReader();
          reader.onloadend = function (e) {
            result = btoa(this.result);
            utils.call(callback, null, result);
          };
          reader.readAsBinaryString(data);
        } else {
          result = data;
          utils.call(callback, null, result);
        }
      } else {
        if (blobSupport) {
          result = data;
        } else {
          data = fixBinary(atob(data));
          result = utils.createBlob([data], {type: type});
        }
        utils.call(callback, null, result);
      }
    };
  };

  api._allDocs = function idb_allDocs(opts, callback) {
    var start = 'startkey' in opts ? opts.startkey : false;
    var end = 'endkey' in opts ? opts.endkey : false;

    var descending = 'descending' in opts ? opts.descending : false;
    descending = descending ? 'prev' : null;

    var keyRange = start && end ? window.IDBKeyRange.bound(start, end)
      : start ? window.IDBKeyRange.lowerBound(start)
      : end ? window.IDBKeyRange.upperBound(end) : null;

    var transaction = idb.transaction([DOC_STORE, BY_SEQ_STORE], 'readonly');
    transaction.oncomplete = function () {
      if ('keys' in opts) {
        opts.keys.forEach(function (key) {
          if (key in resultsMap) {
            results.push(resultsMap[key]);
          } else {
            results.push({"key": key, "error": "not_found"});
          }
        });
        if (opts.descending) {
          results.reverse();
        }
      }
      utils.call(callback, null, {
        total_rows: results.length,
        offset: opts.skip,
        rows: ('limit' in opts) ? results.slice(opts.skip, opts.limit + opts.skip) :
          (opts.skip > 0) ? results.slice(opts.skip) : results
      });
    };

    var oStore = transaction.objectStore(DOC_STORE);
    var oCursor = descending ? oStore.openCursor(keyRange, descending)
      : oStore.openCursor(keyRange);
    var results = [];
    var resultsMap = {};
    oCursor.onsuccess = function (e) {
      if (!e.target.result) {
        return;
      }
      var cursor = e.target.result;
      var metadata = cursor.value;
      // If opts.keys is set we want to filter here only those docs with
      // key in opts.keys. With no performance tests it is difficult to
      // guess if iteration with filter is faster than many single requests
      function allDocsInner(metadata, data) {
        if (utils.isLocalId(metadata.id)) {
          return cursor['continue']();
        }
        var doc = {
          id: metadata.id,
          key: metadata.id,
          value: {
            rev: merge.winningRev(metadata)
          }
        };
        if (opts.include_docs) {
          doc.doc = data;
          doc.doc._rev = merge.winningRev(metadata);
          if (doc.doc._doc_id_rev) {
            delete(doc.doc._doc_id_rev);
          }
          if (opts.conflicts) {
            doc.doc._conflicts = merge.collectConflicts(metadata);
          }
          for (var att in doc.doc._attachments) {
            doc.doc._attachments[att].stub = true;
          }
        }
        if ('keys' in opts) {
          if (opts.keys.indexOf(metadata.id) > -1) {
            if (utils.isDeleted(metadata)) {
              doc.value.deleted = true;
              doc.doc = null;
            }
            resultsMap[doc.id] = doc;
          }
        } else {
          if (!utils.isDeleted(metadata)) {
            results.push(doc);
          }
        }
        cursor['continue']();
      }

      if (!opts.include_docs) {
        allDocsInner(metadata);
      } else {
        var index = transaction.objectStore(BY_SEQ_STORE).index('_doc_id_rev');
        var mainRev = merge.winningRev(metadata);
        var key = metadata.id + "::" + mainRev;
        index.get(key).onsuccess = function (event) {
          allDocsInner(cursor.value, event.target.result);
        };
      }
    };
  };

  api._info = function idb_info(callback) {
    var count = 0;
    var update_seq = 0;
    var txn = idb.transaction([DOC_STORE, META_STORE], 'readonly');

    function fetchUpdateSeq(e) {
      update_seq = e.target.result && e.target.result.updateSeq || 0;
    }

    function countDocs(e) {
      var cursor = e.target.result;
      if (!cursor) {
        txn.objectStore(META_STORE).get(META_STORE).onsuccess = fetchUpdateSeq;
        return;
      }
      if (cursor.value.deleted !== true) {
        count++;
      }
      cursor['continue']();
    }

    txn.oncomplete = function () {
      callback(null, {
        db_name: name,
        doc_count: count,
        update_seq: update_seq
      });
    };

    txn.objectStore(DOC_STORE).openCursor().onsuccess = countDocs;
  };

  api._changes = function idb_changes(opts) {
    //console.log(name + ': Start Changes Feed: continuous=' + opts.continuous);

    if (opts.continuous) {
      var id = name + ':' + utils.uuid();
      opts.cancelled = false;
      IdbPouch.Changes.addListener(name, id, api, opts);
      IdbPouch.Changes.notify(name);
      return {
        cancel: function () {
          //console.log(name + ': Cancel Changes Feed');
          opts.cancelled = true;
          IdbPouch.Changes.removeListener(name, id);
        }
      };
    }

    var descending = opts.descending ? 'prev' : null;
    var last_seq = 0;

    // Ignore the `since` parameter when `descending` is true
    opts.since = opts.since && !descending ? opts.since : 0;

    var results = [], resultIndices = {}, dedupResults = [];
    var txn;

    function fetchChanges() {
      txn = idb.transaction([DOC_STORE, BY_SEQ_STORE]);
      txn.oncomplete = onTxnComplete;

      var req;

      if (descending) {
        req = txn.objectStore(BY_SEQ_STORE)
            .openCursor(window.IDBKeyRange.lowerBound(opts.since, true), descending);
      } else {
        req = txn.objectStore(BY_SEQ_STORE)
            .openCursor(window.IDBKeyRange.lowerBound(opts.since, true));
      }

      req.onsuccess = onsuccess;
      req.onerror = onerror;
    }

    fetchChanges();

    function onsuccess(event) {
      if (!event.target.result) {
        // Filter out null results casued by deduping
        for (var i = 0, l = results.length; i < l; i++) {
          var result = results[i];
          if (result) {
            dedupResults.push(result);
          }
        }
        return false;
      }

      var cursor = event.target.result;

      // Try to pre-emptively dedup to save us a bunch of idb calls
      var changeId = cursor.value._id;
      var changeIdIndex = resultIndices[changeId];
      if (changeIdIndex !== undefined) {
        results[changeIdIndex].seq = cursor.key;
        // update so it has the later sequence number
        results.push(results[changeIdIndex]);
        results[changeIdIndex] = null;
        resultIndices[changeId] = results.length - 1;
        return cursor['continue']();
      }

      var index = txn.objectStore(DOC_STORE);
      index.get(cursor.value._id).onsuccess = function (event) {
        var metadata = event.target.result;
        if (utils.isLocalId(metadata.id)) {
          return cursor['continue']();
        }

        if (last_seq < metadata.seq) {
          last_seq = metadata.seq;
        }

        var mainRev = merge.winningRev(metadata);
        var key = metadata.id + "::" + mainRev;
        var index = txn.objectStore(BY_SEQ_STORE).index('_doc_id_rev');
        index.get(key).onsuccess = function (docevent) {
          var doc = docevent.target.result;
          delete doc['_doc_id_rev'];

          doc._rev = mainRev;
          var change = opts.processChange(doc, metadata, opts);
          change.seq = cursor.key;

          // Dedupe the changes feed
          var changeId = change.id, changeIdIndex = resultIndices[changeId];
          if (changeIdIndex !== undefined) {
            results[changeIdIndex] = null;
          }
          results.push(change);
          resultIndices[changeId] = results.length - 1;
          cursor['continue']();
        };
      };
    }

    function onTxnComplete() {
      utils.processChanges(opts, dedupResults, last_seq);
    }

    function onerror(error) {
      // TODO: shouldn't we pass some params here?
      utils.call(opts.complete);
    }
  };

  api._close = function (callback) {
    if (idb === null) {
      return utils.call(callback, errors.NOT_OPEN);
    }

    // https://developer.mozilla.org/en-US/docs/IndexedDB/IDBDatabase#close
    // "Returns immediately and closes the connection in a separate thread..."
    idb.close();
    utils.call(callback, null);
  };

  api._getRevisionTree = function (docId, callback) {
    var txn = idb.transaction([DOC_STORE], 'readonly');
    var req = txn.objectStore(DOC_STORE).get(docId);
    req.onsuccess = function (event) {
      var doc = event.target.result;
      if (!doc) {
        utils.call(callback, errors.MISSING_DOC);
      } else {
        utils.call(callback, null, doc.rev_tree);
      }
    };
  };

  // This function removes revisions of document docId
  // which are listed in revs and sets this document
  // revision to to rev_tree
  api._doCompaction = function (docId, rev_tree, revs, callback) {
    var txn = idb.transaction([DOC_STORE, BY_SEQ_STORE], 'readwrite');

    var index = txn.objectStore(DOC_STORE);
    index.get(docId).onsuccess = function (event) {
      var metadata = event.target.result;
      metadata.rev_tree = rev_tree;

      var count = revs.length;
      revs.forEach(function (rev) {
        var index = txn.objectStore(BY_SEQ_STORE).index('_doc_id_rev');
        var key = docId + "::" + rev;
        index.getKey(key).onsuccess = function (e) {
          var seq = e.target.result;
          if (!seq) {
            return;
          }
          var req = txn.objectStore(BY_SEQ_STORE)['delete'](seq);

          count--;
          if (!count) {
            txn.objectStore(DOC_STORE).put(metadata);
          }
        };
      });
    };
    txn.oncomplete = function () {
      utils.call(callback);
    };
  };

  return api;
}

IdbPouch.valid = function idb_valid() {
  return typeof window !== 'undefined' && !!window.indexedDB;
};

IdbPouch.destroy = function idb_destroy(name, opts, callback) {
  if (!('openReqList' in IdbPouch)) {
    IdbPouch.openReqList = {};
  }
  IdbPouch.Changes.clearListeners(name);

  //Close open request for "name" database to fix ie delay.
  if (IdbPouch.openReqList[name] && IdbPouch.openReqList[name].result) {
    IdbPouch.openReqList[name].result.close();
  }
  var req = window.indexedDB.deleteDatabase(name);

  req.onsuccess = function () {
    //Remove open request from the list.
    if (IdbPouch.openReqList[name]) {
      IdbPouch.openReqList[name] = null;
    }
    utils.call(callback, null);
  };

  req.onerror = idbError(callback);
};

IdbPouch.Changes = new utils.Changes();

module.exports = IdbPouch;

},{"../deps/errors":8,"../merge":13,"../utils":16}],4:[function(_dereq_,module,exports){
'use strict';

var utils = _dereq_('../utils');
var merge = _dereq_('../merge');
var errors = _dereq_('../deps/errors');
function quote(str) {
  return "'" + str + "'";
}

function openDB() {
  if (typeof window !== 'undefined') {
    if (window.navigator && window.navigator.sqlitePlugin && window.navigator.sqlitePlugin.openDatabase) {
      return navigator.sqlitePlugin.openDatabase.apply(navigator.sqlitePlugin, arguments);
    } else if (window.sqlitePlugin && window.sqlitePlugin.openDatabase) {
      return window.sqlitePlugin.openDatabase.apply(window.sqlitePlugin, arguments);
    } else {
      return window.openDatabase.apply(window, arguments);
    }
  }
}

var POUCH_VERSION = 1;
var POUCH_SIZE = 5 * 1024 * 1024;

// The object stores created for each database
// DOC_STORE stores the document meta data, its revision history and state
var DOC_STORE = quote('document-store');
// BY_SEQ_STORE stores a particular version of a document, keyed by its
// sequence id
var BY_SEQ_STORE = quote('by-sequence');
// Where we store attachments
var ATTACH_STORE = quote('attach-store');
var META_STORE = quote('metadata-store');

function unknownError(callback) {
  return function (event) {
    utils.call(callback, {
      status: 500,
      error: event.type,
      reason: event.target
    });
  };
}
function idbError(callback) {
  return function (event) {
    utils.call(callback, errors.error(errors.WSQ_ERROR, event.target, event.type));
  };
}
function webSqlPouch(opts, callback) {

  var api = {};
  var instanceId = null;
  var name = opts.name;

  var db = openDB(name, POUCH_VERSION, name, POUCH_SIZE);
  if (!db) {
    return utils.call(callback, errors.UNKNOWN_ERROR);
  }

  function dbCreated() {
    callback(null, api);
  }

  function setup() {
    db.transaction(function (tx) {
      var meta = 'CREATE TABLE IF NOT EXISTS ' + META_STORE +
        ' (update_seq, dbid)';
      var attach = 'CREATE TABLE IF NOT EXISTS ' + ATTACH_STORE +
        ' (digest, json, body BLOB)';
      var doc = 'CREATE TABLE IF NOT EXISTS ' + DOC_STORE +
        ' (id unique, seq, json, winningseq)';
      var seq = 'CREATE TABLE IF NOT EXISTS ' + BY_SEQ_STORE +
        ' (seq INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, doc_id_rev UNIQUE, json)';

      tx.executeSql(attach);
      tx.executeSql(doc);
      tx.executeSql(seq);
      tx.executeSql(meta);

      var updateseq = 'SELECT update_seq FROM ' + META_STORE;
      tx.executeSql(updateseq, [], function (tx, result) {
        if (!result.rows.length) {
          var initSeq = 'INSERT INTO ' + META_STORE + ' (update_seq) VALUES (?)';
          tx.executeSql(initSeq, [0]);
          return;
        }
      });

      var dbid = 'SELECT dbid FROM ' + META_STORE + ' WHERE dbid IS NOT NULL';
      tx.executeSql(dbid, [], function (tx, result) {
        if (!result.rows.length) {
          var initDb = 'UPDATE ' + META_STORE + ' SET dbid=?';
          instanceId = utils.uuid();
          tx.executeSql(initDb, [instanceId]);
          return;
        }
        instanceId = result.rows.item(0).dbid;
      });
    }, unknownError(callback), dbCreated);
  }
  if (utils.isCordova() && typeof window !== 'undefined') {
    //to wait until custom api is made in pouch.adapters before doing setup
    window.addEventListener(name + '_pouch', function cordova_init() {
      window.removeEventListener(name + '_pouch', cordova_init, false);
      setup();
    }, false);
  } else {
    setup();
  }


  api.type = function () {
    return 'websql';
  };

  api.id = function () {
    return instanceId;
  };

  api._info = function (callback) {
    db.transaction(function (tx) {
      var sql = 'SELECT COUNT(id) AS count FROM ' + DOC_STORE;
      tx.executeSql(sql, [], function (tx, result) {
        var doc_count = result.rows.item(0).count;
        var updateseq = 'SELECT update_seq FROM ' + META_STORE;
        tx.executeSql(updateseq, [], function (tx, result) {
          var update_seq = result.rows.item(0).update_seq;
          callback(null, {
            db_name: name,
            doc_count: doc_count,
            update_seq: update_seq
          });
        });
      });
    });
  };

  api._bulkDocs = function (req, opts, callback) {

    var newEdits = opts.new_edits;
    var userDocs = req.docs;
    var docsWritten = 0;

    // Parse the docs, give them a sequence number for the result
    var docInfos = userDocs.map(function (doc, i) {
      var newDoc = utils.parseDoc(doc, newEdits);
      newDoc._bulk_seq = i;
      return newDoc;
    });

    var docInfoErrors = docInfos.filter(function (docInfo) {
      return docInfo.error;
    });
    if (docInfoErrors.length) {
      return utils.call(callback, docInfoErrors[0]);
    }

    var tx;
    var results = [];
    var fetchedDocs = {};

    function sortByBulkSeq(a, b) {
      return a._bulk_seq - b._bulk_seq;
    }

    function complete(event) {
      var aresults = [];
      results.sort(sortByBulkSeq);
      results.forEach(function (result) {
        delete result._bulk_seq;
        if (result.error) {
          aresults.push(result);
          return;
        }
        var metadata = result.metadata;
        var rev = merge.winningRev(metadata);

        aresults.push({
          ok: true,
          id: metadata.id,
          rev: rev
        });

        if (utils.isLocalId(metadata.id)) {
          return;
        }

        docsWritten++;

        webSqlPouch.Changes.notify(name);
        webSqlPouch.Changes.notifyLocalWindows(name);
      });

      var updateseq = 'SELECT update_seq FROM ' + META_STORE;
      tx.executeSql(updateseq, [], function (tx, result) {
        var update_seq = result.rows.item(0).update_seq + docsWritten;
        var sql = 'UPDATE ' + META_STORE + ' SET update_seq=?';
        tx.executeSql(sql, [update_seq], function () {
          utils.call(callback, null, aresults);
        });
      });
    }

    function preprocessAttachment(att, finish) {
      if (att.stub) {
        return finish();
      }
      if (typeof att.data === 'string') {
        try {
          att.data = atob(att.data);
        } catch (e) {
          var err = errors.error(errors.BAD_ARG,
                                "Attachments need to be base64 encoded");
          return utils.call(callback, err);
        }
        att.digest = 'md5-' + utils.Crypto.MD5(att.data);
        return finish();
      }
      var reader = new FileReader();
      reader.onloadend = function (e) {
        att.data = this.result;
        att.digest = 'md5-' + utils.Crypto.MD5(this.result);
        finish();
      };
      reader.readAsBinaryString(att.data);
    }

    function preprocessAttachments(callback) {
      if (!docInfos.length) {
        return callback();
      }

      var docv = 0;
      var recv = 0;

      docInfos.forEach(function (docInfo) {
        var attachments = docInfo.data && docInfo.data._attachments ?
          Object.keys(docInfo.data._attachments) : [];

        if (!attachments.length) {
          return done();
        }

        function processedAttachment() {
          recv++;
          if (recv === attachments.length) {
            done();
          }
        }

        for (var key in docInfo.data._attachments) {
          preprocessAttachment(docInfo.data._attachments[key], processedAttachment);
        }
      });

      function done() {
        docv++;
        if (docInfos.length === docv) {
          callback();
        }
      }
    }

    function writeDoc(docInfo, callback, isUpdate) {

      function finish() {
        var data = docInfo.data;
        var sql = 'INSERT INTO ' + BY_SEQ_STORE + ' (doc_id_rev, json) VALUES (?, ?);';
        tx.executeSql(sql, [data._id + "::" + data._rev,
                            JSON.stringify(data)], dataWritten);
      }

      function collectResults(attachmentErr) {
        if (!err) {
          if (attachmentErr) {
            err = attachmentErr;
            utils.call(callback, err);
          } else if (recv === attachments.length) {
            finish();
          }
        }
      }

      var err = null;
      var recv = 0;

      docInfo.data._id = docInfo.metadata.id;
      docInfo.data._rev = docInfo.metadata.rev;

      if (utils.isDeleted(docInfo.metadata, docInfo.metadata.rev)) {
        docInfo.data._deleted = true;
      }

      var attachments = docInfo.data._attachments ?
        Object.keys(docInfo.data._attachments) : [];

      function attachmentSaved(err) {
        recv++;
        collectResults(err);
      }

      for (var key in docInfo.data._attachments) {
        if (!docInfo.data._attachments[key].stub) {
          var data = docInfo.data._attachments[key].data;
          delete docInfo.data._attachments[key].data;
          var digest = docInfo.data._attachments[key].digest;
          saveAttachment(docInfo, digest, data, attachmentSaved);
        } else {
          recv++;
          collectResults();
        }
      }

      if (!attachments.length) {
        finish();
      }

      function dataWritten(tx, result) {
        var seq = docInfo.metadata.seq = result.insertId;
        delete docInfo.metadata.rev;

        var mainRev = merge.winningRev(docInfo.metadata);

        var sql = isUpdate ?
          'UPDATE ' + DOC_STORE + ' SET seq=?, json=?, winningseq=(SELECT seq FROM ' +
          BY_SEQ_STORE + ' WHERE doc_id_rev=?) WHERE id=?' :
          'INSERT INTO ' + DOC_STORE + ' (id, seq, winningseq, json) VALUES (?, ?, ?, ?);';
        var metadataStr = JSON.stringify(docInfo.metadata);
        var key = docInfo.metadata.id + "::" + mainRev;
        var params = isUpdate ?
          [seq, metadataStr, key, docInfo.metadata.id] :
          [docInfo.metadata.id, seq, seq, metadataStr];
        tx.executeSql(sql, params, function (tx, result) {
          results.push(docInfo);
          utils.call(callback, null);
        });
      }
    }

    function updateDoc(oldDoc, docInfo) {
      var merged = merge.merge(oldDoc.rev_tree, docInfo.metadata.rev_tree[0], 1000);
      var inConflict = (utils.isDeleted(oldDoc) &&
                        utils.isDeleted(docInfo.metadata)) ||
        (!utils.isDeleted(oldDoc) &&
         newEdits && merged.conflicts !== 'new_leaf');

      if (inConflict) {
        results.push(makeErr(errors.REV_CONFLICT, docInfo._bulk_seq));
        return processDocs();
      }

      docInfo.metadata.rev_tree = merged.tree;
      writeDoc(docInfo, processDocs, true);
    }

    function insertDoc(docInfo) {
      // Cant insert new deleted documents
      if ('was_delete' in opts && utils.isDeleted(docInfo.metadata)) {
        results.push(errors.MISSING_DOC);
        return processDocs();
      }
      writeDoc(docInfo, processDocs, false);
    }

    function processDocs() {
      if (!docInfos.length) {
        return complete();
      }
      var currentDoc = docInfos.shift();
      var id = currentDoc.metadata.id;
      if (id in fetchedDocs) {
        updateDoc(fetchedDocs[id], currentDoc);
      } else {
        // if we have newEdits=false then we can update the same
        // document twice in a single bulk docs call
        fetchedDocs[id] = currentDoc.metadata;
        insertDoc(currentDoc);
      }
    }

    // Insert sequence number into the error so we can sort later
    function makeErr(err, seq) {
      err._bulk_seq = seq;
      return err;
    }

    function saveAttachment(docInfo, digest, data, callback) {
      var ref = [docInfo.metadata.id, docInfo.metadata.rev].join('@');
      var newAtt = {digest: digest};
      var sql = 'SELECT digest, json FROM ' + ATTACH_STORE + ' WHERE digest=?';
      tx.executeSql(sql, [digest], function (tx, result) {
        if (!result.rows.length) {
          newAtt.refs = {};
          newAtt.refs[ref] = true;
          sql = 'INSERT INTO ' + ATTACH_STORE + '(digest, json, body) VALUES (?, ?, ?)';
          tx.executeSql(sql, [digest, JSON.stringify(newAtt), data], function () {
            utils.call(callback, null);
          });
        } else {
          newAtt.refs = JSON.parse(result.rows.item(0).json).refs;
          sql = 'UPDATE ' + ATTACH_STORE + ' SET json=?, body=? WHERE digest=?';
          tx.executeSql(sql, [JSON.stringify(newAtt), data, digest], function () {
            utils.call(callback, null);
          });
        }
      });
    }

    function metadataFetched(tx, results) {
      for (var j = 0; j < results.rows.length; j++) {
        var row = results.rows.item(j);
        fetchedDocs[row.id] = JSON.parse(row.json);
      }
      processDocs();
    }

    preprocessAttachments(function () {
      db.transaction(function (txn) {
        tx = txn;
        var ids = '(' + docInfos.map(function (d) {
          return quote(d.metadata.id);
        }).join(',') + ')';
        var sql = 'SELECT * FROM ' + DOC_STORE + ' WHERE id IN ' + ids;
        tx.executeSql(sql, [], metadataFetched);
      }, unknownError(callback));
    });
  };

  api._get = function (id, opts, callback) {
    var doc;
    var metadata;
    var err;
    if (!opts.ctx) {
      db.transaction(function (txn) {
        opts.ctx = txn;
        api._get(id, opts, callback);
      });
      return;
    }
    var tx = opts.ctx;

    function finish() {
      utils.call(callback, err, {doc: doc, metadata: metadata, ctx: tx});
    }

    var sql = 'SELECT * FROM ' + DOC_STORE + ' WHERE id=?';
    tx.executeSql(sql, [id], function (a, results) {
      if (!results.rows.length) {
        err = errors.MISSING_DOC;
        return finish();
      }
      metadata = JSON.parse(results.rows.item(0).json);
      if (utils.isDeleted(metadata) && !opts.rev) {
        err = errors.error(errors.MISSING_DOC, "deleted");
        return finish();
      }

      var rev = merge.winningRev(metadata);
      var key = opts.rev ? opts.rev : rev;
      key = metadata.id + '::' + key;
      var sql = 'SELECT * FROM ' + BY_SEQ_STORE + ' WHERE doc_id_rev=?';
      tx.executeSql(sql, [key], function (tx, results) {
        if (!results.rows.length) {
          err = errors.MISSING_DOC;
          return finish();
        }
        doc = JSON.parse(results.rows.item(0).json);

        finish();
      });
    });
  };

  function makeRevs(arr) {
    return arr.map(function (x) { return {rev: x.rev}; });
  }

  api._allDocs = function (opts, callback) {
    var results = [];
    var resultsMap = {};
    var start = 'startkey' in opts ? opts.startkey : false;
    var end = 'endkey' in opts ? opts.endkey : false;
    var descending = 'descending' in opts ? opts.descending : false;
    var sql = 'SELECT ' + DOC_STORE + '.id, ' + BY_SEQ_STORE + '.seq, ' +
      BY_SEQ_STORE + '.json AS data, ' + DOC_STORE + '.json AS metadata FROM ' +
      BY_SEQ_STORE + ' JOIN ' + DOC_STORE + ' ON ' + BY_SEQ_STORE + '.seq = ' +
      DOC_STORE + '.winningseq';

    var sqlArgs = [];
    if ('keys' in opts) {
      sql += ' WHERE ' + DOC_STORE + '.id IN (' + opts.keys.map(function () {
        return '?';
      }).join(',') + ')';
      sqlArgs = sqlArgs.concat(opts.keys);
    } else {
      if (start) {
        sql += ' WHERE ' + DOC_STORE + '.id >= ?';
        sqlArgs.push(start);
      }
      if (end) {
        sql += (start ? ' AND ' : ' WHERE ') + DOC_STORE + '.id <= ?';
        sqlArgs.push(end);
      }
      sql += ' ORDER BY ' + DOC_STORE + '.id ' + (descending ? 'DESC' : 'ASC');
    }

    db.transaction(function (tx) {
      tx.executeSql(sql, sqlArgs, function (tx, result) {
        for (var i = 0, l = result.rows.length; i < l; i++) {
          var doc = result.rows.item(i);
          var metadata = JSON.parse(doc.metadata);
          var data = JSON.parse(doc.data);
          if (!(utils.isLocalId(metadata.id))) {
            doc = {
              id: metadata.id,
              key: metadata.id,
              value: {rev: merge.winningRev(metadata)}
            };
            if (opts.include_docs) {
              doc.doc = data;
              doc.doc._rev = merge.winningRev(metadata);
              if (opts.conflicts) {
                doc.doc._conflicts = merge.collectConflicts(metadata);
              }
              for (var att in doc.doc._attachments) {
                doc.doc._attachments[att].stub = true;
              }
            }
            if ('keys' in opts) {
              if (opts.keys.indexOf(metadata.id) > -1) {
                if (utils.isDeleted(metadata)) {
                  doc.value.deleted = true;
                  doc.doc = null;
                }
                resultsMap[doc.id] = doc;
              }
            } else {
              if (!utils.isDeleted(metadata)) {
                results.push(doc);
              }
            }
          }
        }
      });
    }, unknownError(callback), function () {
      if ('keys' in opts) {
        opts.keys.forEach(function (key) {
          if (key in resultsMap) {
            results.push(resultsMap[key]);
          } else {
            results.push({"key": key, "error": "not_found"});
          }
        });
        if (opts.descending) {
          results.reverse();
        }
      }
      utils.call(callback, null, {
        total_rows: results.length,
        offset: opts.skip,
        rows: ('limit' in opts) ? results.slice(opts.skip, opts.limit + opts.skip) :
          (opts.skip > 0) ? results.slice(opts.skip) : results
      });
    });
  };

  api._changes = function idb_changes(opts) {


    //console.log(name + ': Start Changes Feed: continuous=' + opts.continuous);


    if (opts.continuous) {
      var id = name + ':' + utils.uuid();
      opts.cancelled = false;
      webSqlPouch.Changes.addListener(name, id, api, opts);
      webSqlPouch.Changes.notify(name);
      return {
        cancel: function () {
          //console.log(name + ': Cancel Changes Feed');
          opts.cancelled = true;
          webSqlPouch.Changes.removeListener(name, id);
        }
      };
    }

    var descending = opts.descending;

    // Ignore the `since` parameter when `descending` is true
    opts.since = opts.since && !descending ? opts.since : 0;

    var results = [];
    var txn;

    function fetchChanges() {
      var sql = 'SELECT ' + DOC_STORE + '.id, ' + BY_SEQ_STORE + '.seq, ' +
        BY_SEQ_STORE + '.json AS data, ' + DOC_STORE + '.json AS metadata FROM ' +
        BY_SEQ_STORE + ' JOIN ' + DOC_STORE + ' ON ' + BY_SEQ_STORE + '.seq = ' +
        DOC_STORE + '.winningseq WHERE ' + DOC_STORE + '.seq > ' + opts.since +
        ' ORDER BY ' + DOC_STORE + '.seq ' + (descending ? 'DESC' : 'ASC');

      db.transaction(function (tx) {
        tx.executeSql(sql, [], function (tx, result) {
          var last_seq = 0;
          for (var i = 0, l = result.rows.length; i < l; i++) {
            var res = result.rows.item(i);
            var metadata = JSON.parse(res.metadata);
            if (!utils.isLocalId(metadata.id)) {
              if (last_seq < res.seq) {
                last_seq = res.seq;
              }
              var doc = JSON.parse(res.data);
              var change = opts.processChange(doc, metadata, opts);
              change.seq = res.seq;

              results.push(change);
            }
          }
          utils.processChanges(opts, results, last_seq);
        });
      });
    }

    fetchChanges();
  };

  api._close = function (callback) {
    //WebSQL databases do not need to be closed
    utils.call(callback, null);
  };

  api._getAttachment = function (attachment, opts, callback) {
    var res;
    var tx = opts.ctx;
    var digest = attachment.digest;
    var type = attachment.content_type;
    var sql = 'SELECT body FROM ' + ATTACH_STORE + ' WHERE digest=?';
    tx.executeSql(sql, [digest], function (tx, result) {
      var data = result.rows.item(0).body;
      if (opts.encode) {
        res = btoa(data);
      } else {
        res = utils.createBlob([data], {type: type});
      }
      utils.call(callback, null, res);
    });
  };

  api._getRevisionTree = function (docId, callback) {
    db.transaction(function (tx) {
      var sql = 'SELECT json AS metadata FROM ' + DOC_STORE + ' WHERE id = ?';
      tx.executeSql(sql, [docId], function (tx, result) {
        if (!result.rows.length) {
          utils.call(callback, errors.MISSING_DOC);
        } else {
          var data = JSON.parse(result.rows.item(0).metadata);
          utils.call(callback, null, data.rev_tree);
        }
      });
    });
  };

  api._doCompaction = function (docId, rev_tree, revs, callback) {
    db.transaction(function (tx) {
      var sql = 'SELECT json AS metadata FROM ' + DOC_STORE + ' WHERE id = ?';
      tx.executeSql(sql, [docId], function (tx, result) {
        if (!result.rows.length) {
          return utils.call(callback);
        }
        var metadata = JSON.parse(result.rows.item(0).metadata);
        metadata.rev_tree = rev_tree;

        var sql = 'DELETE FROM ' + BY_SEQ_STORE + ' WHERE doc_id_rev IN (' +
          revs.map(function (rev) {return quote(docId + '::' + rev); }).join(',') + ')';

        tx.executeSql(sql, [], function (tx, result) {
          var sql = 'UPDATE ' + DOC_STORE + ' SET json = ? WHERE id = ?';

          tx.executeSql(sql, [JSON.stringify(metadata), docId], function (tx, result) {
            callback();
          });
        });
      });
    });
  };

  return api;
}

webSqlPouch.valid = function () {
  if (typeof window !== 'undefined') {
    if (window.navigator && window.navigator.sqlitePlugin && window.navigator.sqlitePlugin.openDatabase) {
      return true;
    } else if (window.sqlitePlugin && window.sqlitePlugin.openDatabase) {
      return true;
    } else if (window.openDatabase) {
      return true;
    }
  }
  return false;
};

webSqlPouch.destroy = function (name, opts, callback) {
  var db = openDB(name, POUCH_VERSION, name, POUCH_SIZE);
  db.transaction(function (tx) {
    tx.executeSql('DROP TABLE IF EXISTS ' + DOC_STORE, []);
    tx.executeSql('DROP TABLE IF EXISTS ' + BY_SEQ_STORE, []);
    tx.executeSql('DROP TABLE IF EXISTS ' + ATTACH_STORE, []);
    tx.executeSql('DROP TABLE IF EXISTS ' + META_STORE, []);
  }, unknownError(callback), function () {
    utils.call(callback, null);
  });
};

webSqlPouch.Changes = new utils.Changes();

module.exports = webSqlPouch;

},{"../deps/errors":8,"../merge":13,"../utils":16}],5:[function(_dereq_,module,exports){
"use strict";

var Adapter = _dereq_('./adapter')(PouchDB);
function PouchDB(name, opts, callback) {

  if (!(this instanceof PouchDB)) {
    return new PouchDB(name, opts, callback);
  }

  if (typeof opts === 'function' || typeof opts === 'undefined') {
    callback = opts;
    opts = {};
  }

  if (typeof name === 'object') {
    opts = name;
    name = undefined;
  }

  if (typeof callback === 'undefined') {
    callback = function () {};
  }

  var originalName = opts.name || name;
  var backend = PouchDB.parseAdapter(originalName);
  
  opts.originalName = originalName;
  opts.name = backend.name;
  opts.adapter = opts.adapter || backend.adapter;

  if (!PouchDB.adapters[opts.adapter]) {
    throw 'Adapter is missing';
  }

  if (!PouchDB.adapters[opts.adapter].valid()) {
    throw 'Invalid Adapter';
  }

  var adapter = new Adapter(opts, function (err, db) {
    if (err) {
      if (callback) {
        callback(err);
      }
      return;
    }

    for (var plugin in PouchDB.plugins) {
      // In future these will likely need to be async to allow the plugin
      // to initialise
      var pluginObj = PouchDB.plugins[plugin](db);
      for (var api in pluginObj) {
        // We let things like the http adapter use its own implementation
        // as it shares a lot of code
        if (!(api in db)) {
          db[api] = pluginObj[api];
        }
      }
    }
    db.taskqueue.ready(true);
    db.taskqueue.execute(db);
    callback(null, db);
  });
  for (var j in adapter) {
    this[j] = adapter[j];
  }
  for (var plugin in PouchDB.plugins) {
    // In future these will likely need to be async to allow the plugin
    // to initialise
    var pluginObj = PouchDB.plugins[plugin](this);
    for (var api in pluginObj) {
      // We let things like the http adapter use its own implementation
      // as it shares a lot of code
      if (!(api in this)) {
        this[api] = pluginObj[api];
      }
    }
  }
}

module.exports = PouchDB;
},{"./adapter":1}],6:[function(_dereq_,module,exports){
var process=_dereq_("__browserify_process");"use strict";

var request = _dereq_('request');
var extend = _dereq_('./extend.js');
var createBlob = _dereq_('./blob.js');
var errors = _dereq_('./errors');

function ajax(options, callback) {

  if (typeof options === "function") {
    callback = options;
    options = {};
  }

  function call(fun) {
    /* jshint validthis: true */
    var args = Array.prototype.slice.call(arguments, 1);
    if (typeof fun === typeof Function) {
      fun.apply(this, args);
    }
  }

  var defaultOptions = {
    method : "GET",
    headers: {},
    json: true,
    processData: true,
    timeout: 10000
  };

  options = extend(true, defaultOptions, options);

  function onSuccess(obj, resp, cb) {
    if (!options.binary && !options.json && options.processData &&
      typeof obj !== 'string') {
      obj = JSON.stringify(obj);
    } else if (!options.binary && options.json && typeof obj === 'string') {
      try {
        obj = JSON.parse(obj);
      } catch (e) {
        // Probably a malformed JSON from server
        call(cb, e);
        return;
      }
    }
    if (Array.isArray(obj)) {
      obj = obj.map(function (v) {
        var obj;
        if (v.ok) {
          return v;
        } else if (v.error && v.error === 'conflict') {
          obj = errors.REV_CONFLICT;
          obj.id = v.id;
          return obj;
        } else if (v.missing) {
          obj = errors.MISSING_DOC;
          obj.missing = v.missing;
          return obj;
        }
      });
    }
    call(cb, null, obj, resp);
  }

  function onError(err, cb) {
    var errParsed, errObj, errType, key;
    try {
      errParsed = JSON.parse(err.responseText);
      //would prefer not to have a try/catch clause
      for (key in errors) {
        if (errors[key].name === errParsed.error) {
          errType = errors[key];
          break;
        }
      }
      errType = errType || errors.UNKNOWN_ERROR;
      errObj = errors.error(errType, errParsed.reason);
    } catch (e) {
      errObj = errors.error(errors.UNKNOWN_ERROR);
    }
    call(cb, errObj);
  }

  if (process.browser && typeof XMLHttpRequest !== 'undefined') {
    var timer, timedout = false;
    var xhr = new XMLHttpRequest();

    xhr.open(options.method, options.url);
    xhr.withCredentials = true;

    if (options.json) {
      options.headers.Accept = 'application/json';
      options.headers['Content-Type'] = options.headers['Content-Type'] ||
        'application/json';
      if (options.body && options.processData && typeof options.body !== "string") {
        options.body = JSON.stringify(options.body);
      }
    }

    if (options.binary) {
      xhr.responseType = 'arraybuffer';
    }

    var createCookie = function (name, value, days) {
      var expires = "";
      if (days) {
        var date = new Date();
        date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
        expires = "; expires=" + date.toGMTString();
      }
      document.cookie = name + "=" + value + expires + "; path=/";
    };

    for (var key in options.headers) {
      if (key === 'Cookie') {
        var cookie = options.headers[key].split('=');
        createCookie(cookie[0], cookie[1], 10);
      } else {
        xhr.setRequestHeader(key, options.headers[key]);
      }
    }

    if (!("body" in options)) {
      options.body = null;
    }

    var abortReq = function () {
      timedout = true;
      xhr.abort();
      call(onError, xhr, callback);
    };

    xhr.onreadystatechange = function () {
      if (xhr.readyState !== 4 || timedout) {
        return;
      }
      clearTimeout(timer);
      if (xhr.status >= 200 && xhr.status < 300) {
        var data;
        if (options.binary) {
          data = createBlob([xhr.response || ''], {
            type: xhr.getResponseHeader('Content-Type')
          });
        } else {
          data = xhr.responseText;
        }
        call(onSuccess, data, xhr, callback);
      } else {
        call(onError, xhr, callback);
      }
    };

    if (options.timeout > 0) {
      timer = setTimeout(abortReq, options.timeout);
      xhr.upload.onprogress = xhr.onprogress = function () {
        clearTimeout(timer);
        timer = setTimeout(abortReq, options.timeout);
      };
    }
    xhr.send(options.body);
    return {abort: abortReq};

  } else {

    if (options.json) {
      if (!options.binary) {
        options.headers.Accept = 'application/json';
      }
      options.headers['Content-Type'] = options.headers['Content-Type'] ||
        'application/json';
    }

    if (options.binary) {
      options.encoding = null;
      options.json = false;
    }

    if (!options.processData) {
      options.json = false;
    }

    return request(options, function (err, response, body) {
      if (err) {
        err.status = response ? response.statusCode : 400;
        return call(onError, err, callback);
      }
      var error;
      var content_type = response.headers['content-type'];
      var data = (body || '');

      // CouchDB doesn't always return the right content-type for JSON data, so
      // we check for ^{ and }$ (ignoring leading/trailing whitespace)
      if (!options.binary && (options.json || !options.processData) &&
          typeof data !== 'object' &&
          (/json/.test(content_type) ||
           (/^[\s]*\{/.test(data) && /\}[\s]*$/.test(data)))) {
        data = JSON.parse(data);
      }

      if (response.statusCode >= 200 && response.statusCode < 300) {
        call(onSuccess, data, response, callback);
      }
      else {
        if (options.binary) {
          data = JSON.parse(data.toString());
        }
        if (data.reason === 'missing') {
          error = errors.MISSING_DOC;
        } else if (data.reason === 'no_db_file') {
          error = errors.error(errors.DB_MISSING, data.reason);
        } else if (data.error === 'conflict') {
          error = errors.REV_CONFLICT;
        } else {
          error = errors.error(errors.UNKNOWN_ERROR, data.reason, data.error);
        }
        error.status = response.statusCode;
        call(callback, error);
      }
    });
  }
}

module.exports = ajax;

},{"./blob.js":7,"./errors":8,"./extend.js":9,"__browserify_process":19,"request":18}],7:[function(_dereq_,module,exports){
"use strict";

//Abstracts constructing a Blob object, so it also works in older
//browsers that don't support the native Blob constructor. (i.e.
//old QtWebKit versions, at least).
function createBlob(parts, properties) {
  parts = parts || [];
  properties = properties || {};
  try {
    return new Blob(parts, properties);
  } catch (e) {
    if (e.name !== "TypeError") {
      throw e;
    }
    var BlobBuilder = window.BlobBuilder || window.MSBlobBuilder || window.MozBlobBuilder || window.WebKitBlobBuilder;
    var builder = new BlobBuilder();
    for (var i = 0; i < parts.length; i += 1) {
      builder.append(parts[i]);
    }
    return builder.getBlob(properties.type);
  }
}

module.exports = createBlob;


},{}],8:[function(_dereq_,module,exports){
"use strict";

function PouchError(opts) {
  this.status = opts.status;
  this.name = opts.error;
  this.message = opts.reason;
  this.error = true;
}
PouchError.prototype = new Error();


exports.MISSING_BULK_DOCS = new PouchError({
  status: 400,
  error: 'bad_request',
  reason: "Missing JSON list of 'docs'"
});
exports.MISSING_DOC = new PouchError({ 
  status: 404,
  error: 'not_found',
  reason: 'missing'
});
exports.REV_CONFLICT = new PouchError({
  status: 409,
  error: 'conflict',
  reason: 'Document update conflict'
});
exports.INVALID_ID = new PouchError({
  status: 400,
  error: 'invalid_id',
  reason: '_id field must contain a string'
});
exports.MISSING_ID = new PouchError({
  status: 412,
  error: 'missing_id',
  reason: '_id is required for puts'
});
exports.RESERVED_ID = new PouchError({
  status: 400,
  error: 'bad_request',
  reason: 'Only reserved document ids may start with underscore.'
});
exports.NOT_OPEN = new PouchError({
  status: 412,
  error: 'precondition_failed',
  reason: 'Database not open so cannot close'
});
exports.UNKNOWN_ERROR = new PouchError({
  status: 500,
  error: 'unknown_error',
  reason: 'Database encountered an unknown error'
});
exports.BAD_ARG = new PouchError({
  status: 500,
  error: 'badarg',
  reason: 'Some query argument is invalid'
});
exports.INVALID_REQUEST = new PouchError({
  status: 400,
  error: 'invalid_request',
  reason: 'Request was invalid'
});
exports.QUERY_PARSE_ERROR = new PouchError({
  status: 400,
  error: 'query_parse_error',
  reason: 'Some query parameter is invalid'
});
exports.DOC_VALIDATION = new PouchError({
  status: 500,
  error: 'doc_validation',
  reason: 'Bad special document member'
});
exports.BAD_REQUEST = new PouchError({
  status: 400,
  error: 'bad_request',
  reason: 'Something wrong with the request'
});
exports.NOT_AN_OBJECT = new PouchError({
  status: 400,
  error: 'bad_request',
  reason: 'Document must be a JSON object'
});
exports.DB_MISSING = new PouchError({
  status: 404,
  error: 'not_found',
  reason: 'Database not found'
});
exports.IDB_ERROR = new PouchError({
  status: 500,
  error: 'indexed_db_went_bad',
  reason: 'unknown'
});
exports.WSQ_ERROR = new PouchError({
  status: 500,
  error: 'web_sql_went_bad',
  reason: 'unknown'
});
exports.LDB_ERROR = new PouchError({
  status: 500,
  error: 'levelDB_went_went_bad',
  reason: 'unknown'
});
exports.error = function (error, reason, name) {
  function CustomPouchError(msg) {
    this.message = reason;
    if (name) {
      this.name = name;
    }
  }
  CustomPouchError.prototype = error;
  return new CustomPouchError(reason);
};
},{}],9:[function(_dereq_,module,exports){
"use strict";

// Extends method
// (taken from http://code.jquery.com/jquery-1.9.0.js)
// Populate the class2type map
var class2type = {};

var types = ["Boolean", "Number", "String", "Function", "Array", "Date", "RegExp", "Object", "Error"];
for (var i = 0; i < types.length; i++) {
  var typename = types[i];
  class2type["[object " + typename + "]"] = typename.toLowerCase();
}

var core_toString = class2type.toString;
var core_hasOwn = class2type.hasOwnProperty;

function type(obj) {
  if (obj === null) {
    return String(obj);
  }
  return typeof obj === "object" || typeof obj === "function" ?
    class2type[core_toString.call(obj)] || "object" :
    typeof obj;
}

function isWindow(obj) {
  return obj !== null && obj === obj.window;
}

function isPlainObject(obj) {
  // Must be an Object.
  // Because of IE, we also have to check the presence of the constructor property.
  // Make sure that DOM nodes and window objects don't pass through, as well
  if (!obj || type(obj) !== "object" || obj.nodeType || isWindow(obj)) {
    return false;
  }

  try {
    // Not own constructor property must be Object
    if (obj.constructor &&
      !core_hasOwn.call(obj, "constructor") &&
      !core_hasOwn.call(obj.constructor.prototype, "isPrototypeOf")) {
      return false;
    }
  } catch ( e ) {
    // IE8,9 Will throw exceptions on certain host objects #9897
    return false;
  }

  // Own properties are enumerated firstly, so to speed up,
  // if last one is own, then all properties are own.
  var key;
  for (key in obj) {}

  return key === undefined || core_hasOwn.call(obj, key);
}


function isFunction(obj) {
  return type(obj) === "function";
}

var isArray = Array.isArray || function (obj) {
  return type(obj) === "array";
};

function extend() {
  var options, name, src, copy, copyIsArray, clone,
    target = arguments[0] || {},
    i = 1,
    length = arguments.length,
    deep = false;

  // Handle a deep copy situation
  if (typeof target === "boolean") {
    deep = target;
    target = arguments[1] || {};
    // skip the boolean and the target
    i = 2;
  }

  // Handle case when target is a string or something (possible in deep copy)
  if (typeof target !== "object" && !isFunction(target)) {
    target = {};
  }

  // extend jQuery itself if only one argument is passed
  if (length === i) {
    /* jshint validthis: true */
    target = this;
    --i;
  }

  for (; i < length; i++) {
    // Only deal with non-null/undefined values
    if ((options = arguments[i]) != null) {
      // Extend the base object
      for (name in options) {
        src = target[name];
        copy = options[name];

        // Prevent never-ending loop
        if (target === copy) {
          continue;
        }

        // Recurse if we're merging plain objects or arrays
        if (deep && copy && (isPlainObject(copy) || (copyIsArray = isArray(copy)))) {
          if (copyIsArray) {
            copyIsArray = false;
            clone = src && isArray(src) ? src : [];

          } else {
            clone = src && isPlainObject(src) ? src : {};
          }

          // Never move original objects, clone them
          target[name] = extend(deep, clone, copy);

        // Don't bring in undefined values
        } else if (copy !== undefined) {
          if (!(isArray(options) && isFunction(copy))) {
            target[name] = copy;
          }
        }
      }
    }
  }

  // Return the modified object
  return target;
}


module.exports = extend;


},{}],10:[function(_dereq_,module,exports){
var process=_dereq_("__browserify_process");"use strict";

/**
*
*  MD5 (Message-Digest Algorithm)
*
*  For original source see http://www.webtoolkit.info/
*  Download: 15.02.2009 from http://www.webtoolkit.info/javascript-md5.html
*
*  Licensed under CC-BY 2.0 License
*  (http://creativecommons.org/licenses/by/2.0/uk/)
*
**/
var crypto = _dereq_('crypto');

exports.MD5 = function (string) {
  if (!process.browser) {
    return crypto.createHash('md5').update(string).digest('hex');
  }
  function rotateLeft(lValue, iShiftBits) {
    return (lValue<<iShiftBits) | (lValue>>>(32 - iShiftBits));
  }

  function addUnsigned(lX, lY) {
    var lX4, lY4, lX8, lY8, lResult;
    lX8 = (lX & 0x80000000);
    lY8 = (lY & 0x80000000);
    lX4 = (lX & 0x40000000);
    lY4 = (lY & 0x40000000);
    lResult = (lX & 0x3FFFFFFF) + (lY & 0x3FFFFFFF);
    if (lX4 & lY4) {
      return (lResult ^ 0x80000000 ^ lX8 ^ lY8);
    }
    if (lX4 | lY4) {
      if (lResult & 0x40000000) {
        return (lResult ^ 0xC0000000 ^ lX8 ^ lY8);
      } else {
        return (lResult ^ 0x40000000 ^ lX8 ^ lY8);
      }
    } else {
      return (lResult ^ lX8 ^ lY8);
    }
  }

  function f(x, y, z) { return (x & y) | ((~x) & z); }
  function g(x, y, z) { return (x & z) | (y & (~z)); }
  function h(x, y, z) { return (x ^ y ^ z); }
  function i(x, y, z) { return (y ^ (x | (~z))); }

  function ff(a, b, c, d, x, s, ac) {
    a = addUnsigned(a, addUnsigned(addUnsigned(f(b, c, d), x), ac));
    return addUnsigned(rotateLeft(a, s), b);
  }

  function gg(a, b, c, d, x, s, ac) {
    a = addUnsigned(a, addUnsigned(addUnsigned(g(b, c, d), x), ac));
    return addUnsigned(rotateLeft(a, s), b);
  }

  function hh(a, b, c, d, x, s, ac) {
    a = addUnsigned(a, addUnsigned(addUnsigned(h(b, c, d), x), ac));
    return addUnsigned(rotateLeft(a, s), b);
  }

  function ii(a, b, c, d, x, s, ac) {
    a = addUnsigned(a, addUnsigned(addUnsigned(i(b, c, d), x), ac));
    return addUnsigned(rotateLeft(a, s), b);
  }

  function convertToWordArray(string) {
    var lWordCount;
    var lMessageLength = string.length;
    var lNumberOfWords_temp1 = lMessageLength + 8;
    var lNumberOfWords_temp2 = (lNumberOfWords_temp1 - (lNumberOfWords_temp1 % 64)) / 64;
    var lNumberOfWords = (lNumberOfWords_temp2 + 1) * 16;
    var lWordArray = new Array(lNumberOfWords - 1);
    var lBytePosition = 0;
    var lByteCount = 0;
    while (lByteCount < lMessageLength) {
      lWordCount = (lByteCount - (lByteCount % 4)) / 4;
      lBytePosition = (lByteCount % 4) * 8;
      lWordArray[lWordCount] = (lWordArray[lWordCount] | (string.charCodeAt(lByteCount)<<lBytePosition));
      lByteCount++;
    }
    lWordCount = (lByteCount - (lByteCount % 4)) / 4;
    lBytePosition = (lByteCount % 4) * 8;
    lWordArray[lWordCount] = lWordArray[lWordCount] | (0x80<<lBytePosition);
    lWordArray[lNumberOfWords - 2] = lMessageLength<<3;
    lWordArray[lNumberOfWords - 1] = lMessageLength>>>29;
    return lWordArray;
  }

  function wordToHex(lValue) {
    var   wordToHexValue = "",   wordToHexValue_temp = "", lByte, lCount;
    for (lCount = 0;lCount <= 3;lCount++) {
      lByte = (lValue>>>(lCount * 8)) & 255;
      wordToHexValue_temp = "0" + lByte.toString(16);
      wordToHexValue = wordToHexValue + wordToHexValue_temp.substr(wordToHexValue_temp.length - 2, 2);
    }
    return   wordToHexValue;
  }

  //**  function Utf8Encode(string) removed. Aready defined in pidcrypt_utils.js

  var x = [];
  var k, AA, BB, CC, DD, a, b, c, d;
  var S11 = 7, S12 = 12, S13 = 17, S14 = 22;
  var S21 = 5, S22 = 9,  S23 = 14, S24 = 20;
  var S31 = 4, S32 = 11, S33 = 16, S34 = 23;
  var S41 = 6, S42 = 10, S43 = 15, S44 = 21;

  //  string = Utf8Encode(string); #function call removed

  x = convertToWordArray(string);

  a = 0x67452301;
  b = 0xEFCDAB89;
  c = 0x98BADCFE;
  d = 0x10325476;

  for (k = 0;k < x.length;k += 16) {
    AA = a;
    BB = b;
    CC = c;
    DD = d;
    a = ff(a, b, c, d, x[k + 0],  S11, 0xD76AA478);
    d = ff(d, a, b, c, x[k + 1],  S12, 0xE8C7B756);
    c = ff(c, d, a, b, x[k + 2],  S13, 0x242070DB);
    b = ff(b, c, d, a, x[k + 3],  S14, 0xC1BDCEEE);
    a = ff(a, b, c, d, x[k + 4],  S11, 0xF57C0FAF);
    d = ff(d, a, b, c, x[k + 5],  S12, 0x4787C62A);
    c = ff(c, d, a, b, x[k + 6],  S13, 0xA8304613);
    b = ff(b, c, d, a, x[k + 7],  S14, 0xFD469501);
    a = ff(a, b, c, d, x[k + 8],  S11, 0x698098D8);
    d = ff(d, a, b, c, x[k + 9],  S12, 0x8B44F7AF);
    c = ff(c, d, a, b, x[k + 10], S13, 0xFFFF5BB1);
    b = ff(b, c, d, a, x[k + 11], S14, 0x895CD7BE);
    a = ff(a, b, c, d, x[k + 12], S11, 0x6B901122);
    d = ff(d, a, b, c, x[k + 13], S12, 0xFD987193);
    c = ff(c, d, a, b, x[k + 14], S13, 0xA679438E);
    b = ff(b, c, d, a, x[k + 15], S14, 0x49B40821);
    a = gg(a, b, c, d, x[k + 1],  S21, 0xF61E2562);
    d = gg(d, a, b, c, x[k + 6],  S22, 0xC040B340);
    c = gg(c, d, a, b, x[k + 11], S23, 0x265E5A51);
    b = gg(b, c, d, a, x[k + 0],  S24, 0xE9B6C7AA);
    a = gg(a, b, c, d, x[k + 5],  S21, 0xD62F105D);
    d = gg(d, a, b, c, x[k + 10], S22, 0x2441453);
    c = gg(c, d, a, b, x[k + 15], S23, 0xD8A1E681);
    b = gg(b, c, d, a, x[k + 4],  S24, 0xE7D3FBC8);
    a = gg(a, b, c, d, x[k + 9],  S21, 0x21E1CDE6);
    d = gg(d, a, b, c, x[k + 14], S22, 0xC33707D6);
    c = gg(c, d, a, b, x[k + 3],  S23, 0xF4D50D87);
    b = gg(b, c, d, a, x[k + 8],  S24, 0x455A14ED);
    a = gg(a, b, c, d, x[k + 13], S21, 0xA9E3E905);
    d = gg(d, a, b, c, x[k + 2],  S22, 0xFCEFA3F8);
    c = gg(c, d, a, b, x[k + 7],  S23, 0x676F02D9);
    b = gg(b, c, d, a, x[k + 12], S24, 0x8D2A4C8A);
    a = hh(a, b, c, d, x[k + 5],  S31, 0xFFFA3942);
    d = hh(d, a, b, c, x[k + 8],  S32, 0x8771F681);
    c = hh(c, d, a, b, x[k + 11], S33, 0x6D9D6122);
    b = hh(b, c, d, a, x[k + 14], S34, 0xFDE5380C);
    a = hh(a, b, c, d, x[k + 1],  S31, 0xA4BEEA44);
    d = hh(d, a, b, c, x[k + 4],  S32, 0x4BDECFA9);
    c = hh(c, d, a, b, x[k + 7],  S33, 0xF6BB4B60);
    b = hh(b, c, d, a, x[k + 10], S34, 0xBEBFBC70);
    a = hh(a, b, c, d, x[k + 13], S31, 0x289B7EC6);
    d = hh(d, a, b, c, x[k + 0],  S32, 0xEAA127FA);
    c = hh(c, d, a, b, x[k + 3],  S33, 0xD4EF3085);
    b = hh(b, c, d, a, x[k + 6],  S34, 0x4881D05);
    a = hh(a, b, c, d, x[k + 9],  S31, 0xD9D4D039);
    d = hh(d, a, b, c, x[k + 12], S32, 0xE6DB99E5);
    c = hh(c, d, a, b, x[k + 15], S33, 0x1FA27CF8);
    b = hh(b, c, d, a, x[k + 2],  S34, 0xC4AC5665);
    a = ii(a, b, c, d, x[k + 0],  S41, 0xF4292244);
    d = ii(d, a, b, c, x[k + 7],  S42, 0x432AFF97);
    c = ii(c, d, a, b, x[k + 14], S43, 0xAB9423A7);
    b = ii(b, c, d, a, x[k + 5],  S44, 0xFC93A039);
    a = ii(a, b, c, d, x[k + 12], S41, 0x655B59C3);
    d = ii(d, a, b, c, x[k + 3],  S42, 0x8F0CCC92);
    c = ii(c, d, a, b, x[k + 10], S43, 0xFFEFF47D);
    b = ii(b, c, d, a, x[k + 1],  S44, 0x85845DD1);
    a = ii(a, b, c, d, x[k + 8],  S41, 0x6FA87E4F);
    d = ii(d, a, b, c, x[k + 15], S42, 0xFE2CE6E0);
    c = ii(c, d, a, b, x[k + 6],  S43, 0xA3014314);
    b = ii(b, c, d, a, x[k + 13], S44, 0x4E0811A1);
    a = ii(a, b, c, d, x[k + 4],  S41, 0xF7537E82);
    d = ii(d, a, b, c, x[k + 11], S42, 0xBD3AF235);
    c = ii(c, d, a, b, x[k + 2],  S43, 0x2AD7D2BB);
    b = ii(b, c, d, a, x[k + 9],  S44, 0xEB86D391);
    a = addUnsigned(a, AA);
    b = addUnsigned(b, BB);
    c = addUnsigned(c, CC);
    d = addUnsigned(d, DD);
  }
  var temp = wordToHex(a) + wordToHex(b) + wordToHex(c) + wordToHex(d);
  return temp.toLowerCase();
};
},{"__browserify_process":19,"crypto":18}],11:[function(_dereq_,module,exports){
"use strict";

// BEGIN Math.uuid.js

/*!
Math.uuid.js (v1.4)
http://www.broofa.com
mailto:robert@broofa.com

Copyright (c) 2010 Robert Kieffer
Dual licensed under the MIT and GPL licenses.
*/

/*
 * Generate a random uuid.
 *
 * USAGE: Math.uuid(length, radix)
 *   length - the desired number of characters
 *   radix  - the number of allowable values for each character.
 *
 * EXAMPLES:
 *   // No arguments  - returns RFC4122, version 4 ID
 *   >>> Math.uuid()
 *   "92329D39-6F5C-4520-ABFC-AAB64544E172"
 *
 *   // One argument - returns ID of the specified length
 *   >>> Math.uuid(15)     // 15 character ID (default base=62)
 *   "VcydxgltxrVZSTV"
 *
 *   // Two arguments - returns ID of the specified length, and radix. (Radix must be <= 62)
 *   >>> Math.uuid(8, 2)  // 8 character ID (base=2)
 *   "01001010"
 *   >>> Math.uuid(8, 10) // 8 character ID (base=10)
 *   "47473046"
 *   >>> Math.uuid(8, 16) // 8 character ID (base=16)
 *   "098F4D35"
 */


function uuid(len, radix) {
  var chars = uuid.CHARS;
  var uuidInner = [];
  var i;

  radix = radix || chars.length;

  if (len) {
    // Compact form
    for (i = 0; i < len; i++) {
      uuidInner[i] = chars[0 | Math.random() * radix];
    }
  } else {
    // rfc4122, version 4 form
    var r;

    // rfc4122 requires these characters
    uuidInner[8] = uuidInner[13] = uuidInner[18] = uuidInner[23] = '-';
    uuidInner[14] = '4';

    // Fill in random data.  At i==19 set the high bits of clock sequence as
    // per rfc4122, sec. 4.1.5
    for (i = 0; i < 36; i++) {
      if (!uuidInner[i]) {
        r = 0 | Math.random() * 16;
        uuidInner[i] = chars[(i === 19) ? (r & 0x3) | 0x8 : r];
      }
    }
  }

  return uuidInner.join('');
}

uuid.CHARS = (
  '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ' +
  'abcdefghijklmnopqrstuvwxyz'
).split('');

module.exports = uuid;


},{}],12:[function(_dereq_,module,exports){
var process=_dereq_("__browserify_process");"use strict";

var PouchDB = _dereq_('./setup');

module.exports = PouchDB;

PouchDB.ajax = _dereq_('./deps/ajax');
PouchDB.extend = _dereq_('./deps/extend');
PouchDB.utils = _dereq_('./utils');
PouchDB.Errors = _dereq_('./deps/errors');
PouchDB.replicate = _dereq_('./replicate').replicate;
PouchDB.version = _dereq_('./version');
var httpAdapter = _dereq_('./adapters/http');
PouchDB.adapter('http', httpAdapter);
PouchDB.adapter('https', httpAdapter);

PouchDB.adapter('idb', _dereq_('./adapters/idb'));
PouchDB.adapter('websql', _dereq_('./adapters/websql'));
PouchDB.plugin('mapreduce', _dereq_('pouchdb-mapreduce'));

if (!process.browser) {
  var ldbAdapter = _dereq_('./adapters/leveldb');
  PouchDB.adapter('ldb', ldbAdapter);
  PouchDB.adapter('leveldb', ldbAdapter);
}

},{"./adapters/http":2,"./adapters/idb":3,"./adapters/leveldb":18,"./adapters/websql":4,"./deps/ajax":6,"./deps/errors":8,"./deps/extend":9,"./replicate":14,"./setup":15,"./utils":16,"./version":17,"__browserify_process":19,"pouchdb-mapreduce":20}],13:[function(_dereq_,module,exports){
'use strict';

var extend = _dereq_('./deps/extend');


// for a better overview of what this is doing, read:
// https://github.com/apache/couchdb/blob/master/src/couchdb/couch_key_tree.erl
//
// But for a quick intro, CouchDB uses a revision tree to store a documents
// history, A -> B -> C, when a document has conflicts, that is a branch in the
// tree, A -> (B1 | B2 -> C), We store these as a nested array in the format
//
// KeyTree = [Path ... ]
// Path = {pos: position_from_root, ids: Tree}
// Tree = [Key, Opts, [Tree, ...]], in particular single node: [Key, []]

// Turn a path as a flat array into a tree with a single branch
function pathToTree(path) {
  var doc = path.shift();
  var root = [doc.id, doc.opts, []];
  var leaf = root;
  var nleaf;

  while (path.length) {
    doc = path.shift();
    nleaf = [doc.id, doc.opts, []];
    leaf[2].push(nleaf);
    leaf = nleaf;
  }
  return root;
}

// Merge two trees together
// The roots of tree1 and tree2 must be the same revision
function mergeTree(in_tree1, in_tree2) {
  var queue = [{tree1: in_tree1, tree2: in_tree2}];
  var conflicts = false;
  while (queue.length > 0) {
    var item = queue.pop();
    var tree1 = item.tree1;
    var tree2 = item.tree2;

    if (tree1[1].status || tree2[1].status) {
      tree1[1].status = (tree1[1].status ===  'available' ||
                         tree2[1].status === 'available') ? 'available' : 'missing';
    }

    for (var i = 0; i < tree2[2].length; i++) {
      if (!tree1[2][0]) {
        conflicts = 'new_leaf';
        tree1[2][0] = tree2[2][i];
        continue;
      }

      var merged = false;
      for (var j = 0; j < tree1[2].length; j++) {
        if (tree1[2][j][0] === tree2[2][i][0]) {
          queue.push({tree1: tree1[2][j], tree2: tree2[2][i]});
          merged = true;
        }
      }
      if (!merged) {
        conflicts = 'new_branch';
        tree1[2].push(tree2[2][i]);
        tree1[2].sort();
      }
    }
  }
  return {conflicts: conflicts, tree: in_tree1};
}

function doMerge(tree, path, dontExpand) {
  var restree = [];
  var conflicts = false;
  var merged = false;
  var res, branch;

  if (!tree.length) {
    return {tree: [path], conflicts: 'new_leaf'};
  }

  tree.forEach(function (branch) {
    if (branch.pos === path.pos && branch.ids[0] === path.ids[0]) {
      // Paths start at the same position and have the same root, so they need
      // merged
      res = mergeTree(branch.ids, path.ids);
      restree.push({pos: branch.pos, ids: res.tree});
      conflicts = conflicts || res.conflicts;
      merged = true;
    } else if (dontExpand !== true) {
      // The paths start at a different position, take the earliest path and
      // traverse up until it as at the same point from root as the path we want to
      // merge.  If the keys match we return the longer path with the other merged
      // After stemming we dont want to expand the trees

      var t1 = branch.pos < path.pos ? branch : path;
      var t2 = branch.pos < path.pos ? path : branch;
      var diff = t2.pos - t1.pos;

      var candidateParents = [];

      var trees = [];
      trees.push({ids: t1.ids, diff: diff, parent: null, parentIdx: null});
      while (trees.length > 0) {
        var item = trees.pop();
        if (item.diff === 0) {
          if (item.ids[0] === t2.ids[0]) {
            candidateParents.push(item);
          }
          continue;
        }
        if (!item.ids) {
          continue;
        }
        /*jshint loopfunc:true */
        item.ids[2].forEach(function (el, idx) {
          trees.push({ids: el, diff: item.diff - 1, parent: item.ids, parentIdx: idx});
        });
      }

      var el = candidateParents[0];

      if (!el) {
        restree.push(branch);
      } else {
        res = mergeTree(el.ids, t2.ids);
        el.parent[2][el.parentIdx] = res.tree;
        restree.push({pos: t1.pos, ids: t1.ids});
        conflicts = conflicts || res.conflicts;
        merged = true;
      }
    } else {
      restree.push(branch);
    }
  });

  // We didnt find
  if (!merged) {
    restree.push(path);
  }

  restree.sort(function (a, b) {
    return a.pos - b.pos;
  });

  return {
    tree: restree,
    conflicts: conflicts || 'internal_node'
  };
}

// To ensure we dont grow the revision tree infinitely, we stem old revisions
function stem(tree, depth) {
  // First we break out the tree into a complete list of root to leaf paths,
  // we cut off the start of the path and generate a new set of flat trees
  var stemmedPaths = PouchMerge.rootToLeaf(tree).map(function (path) {
    var stemmed = path.ids.slice(-depth);
    return {
      pos: path.pos + (path.ids.length - stemmed.length),
      ids: pathToTree(stemmed)
    };
  });
  // Then we remerge all those flat trees together, ensuring that we dont
  // connect trees that would go beyond the depth limit
  return stemmedPaths.reduce(function (prev, current, i, arr) {
    return doMerge(prev, current, true).tree;
  }, [stemmedPaths.shift()]);
}

var PouchMerge = {};

PouchMerge.merge = function (tree, path, depth) {
  // Ugh, nicer way to not modify arguments in place?
  tree = extend(true, [], tree);
  path = extend(true, {}, path);
  var newTree = doMerge(tree, path);
  return {
    tree: stem(newTree.tree, depth),
    conflicts: newTree.conflicts
  };
};

// We fetch all leafs of the revision tree, and sort them based on tree length
// and whether they were deleted, undeleted documents with the longest revision
// tree (most edits) win
// The final sort algorithm is slightly documented in a sidebar here:
// http://guide.couchdb.org/draft/conflicts.html
PouchMerge.winningRev = function (metadata) {
  var leafs = [];
  PouchMerge.traverseRevTree(metadata.rev_tree,
                              function (isLeaf, pos, id, something, opts) {
    if (isLeaf) {
      leafs.push({pos: pos, id: id, deleted: !!opts.deleted});
    }
  });
  leafs.sort(function (a, b) {
    if (a.deleted !== b.deleted) {
      return a.deleted > b.deleted ? 1 : -1;
    }
    if (a.pos !== b.pos) {
      return b.pos - a.pos;
    }
    return a.id < b.id ? 1 : -1;
  });

  return leafs[0].pos + '-' + leafs[0].id;
};

// Pretty much all below can be combined into a higher order function to
// traverse revisions
// The return value from the callback will be passed as context to all
// children of that node
PouchMerge.traverseRevTree = function (revs, callback) {
  var toVisit = [];

  revs.forEach(function (tree) {
    toVisit.push({pos: tree.pos, ids: tree.ids});
  });
  while (toVisit.length > 0) {
    var node = toVisit.pop();
    var pos = node.pos;
    var tree = node.ids;
    var newCtx = callback(tree[2].length === 0, pos, tree[0], node.ctx, tree[1]);
    /*jshint loopfunc: true */
    tree[2].forEach(function (branch) {
      toVisit.push({pos: pos + 1, ids: branch, ctx: newCtx});
    });
  }
};

PouchMerge.collectLeaves = function (revs) {
  var leaves = [];
  PouchMerge.traverseRevTree(revs, function (isLeaf, pos, id, acc, opts) {
    if (isLeaf) {
      leaves.unshift({rev: pos + "-" + id, pos: pos, opts: opts});
    }
  });
  leaves.sort(function (a, b) {
    return b.pos - a.pos;
  });
  leaves.map(function (leaf) { delete leaf.pos; });
  return leaves;
};

// returns revs of all conflicts that is leaves such that
// 1. are not deleted and
// 2. are different than winning revision
PouchMerge.collectConflicts = function (metadata) {
  var win = PouchMerge.winningRev(metadata);
  var leaves = PouchMerge.collectLeaves(metadata.rev_tree);
  var conflicts = [];
  leaves.forEach(function (leaf) {
    if (leaf.rev !== win && !leaf.opts.deleted) {
      conflicts.push(leaf.rev);
    }
  });
  return conflicts;
};

PouchMerge.rootToLeaf = function (tree) {
  var paths = [];
  PouchMerge.traverseRevTree(tree, function (isLeaf, pos, id, history, opts) {
    history = history ? history.slice(0) : [];
    history.push({id: id, opts: opts});
    if (isLeaf) {
      var rootPos = pos + 1 - history.length;
      paths.unshift({pos: rootPos, ids: history});
    }
    return history;
  });
  return paths;
};


module.exports = PouchMerge;

},{"./deps/extend":9}],14:[function(_dereq_,module,exports){
var process=_dereq_("__browserify_process");'use strict';

var PouchUtils = _dereq_('./utils');
var Pouch = _dereq_('./index');

// We create a basic promise so the caller can cancel the replication possibly
// before we have actually started listening to changes etc
function Promise() {
  var that = this;
  this.cancelled = false;
  this.cancel = function () {
    that.cancelled = true;
  };
}

// The RequestManager ensures that only one database request is active at
// at time, it ensures we dont max out simultaneous HTTP requests and makes
// the replication process easier to reason about

function RequestManager(promise) {
  var queue = [];
  var api = {};
  var processing = false;

  // Add a new request to the queue, if we arent currently processing anything
  // then process it immediately
  api.enqueue = function (fun, args) {
    queue.push({fun: fun, args: args});
    if (!processing) {
      api.process();
    }
  };

  // Process the next request
  api.process = function () {
    if (processing || !queue.length || promise.cancelled) {
      return;
    }
    processing = true;
    var task = queue.shift();
    process.nextTick(function () {
      task.fun.apply(null, task.args);
    });
  };

  // We need to be notified whenever a request is complete to process
  // the next request
  api.notifyRequestComplete = function () {
    processing = false;
    api.process();
  };

  return api;
}

// TODO: check CouchDB's replication id generation, generate a unique id particular
// to this replication

function genReplicationId(src, target, opts) {
  var filterFun = opts.filter ? opts.filter.toString() : '';
  return '_local/' + PouchUtils.Crypto.MD5(src.id() + target.id() + filterFun);
}

// A checkpoint lets us restart replications from when they were last cancelled

function fetchCheckpoint(src, target, id, callback) {
  target.get(id, function (err, targetDoc) {
    if (err && err.status === 404) {
      callback(null, 0);
    } else if (err) {
      callback(err);
    } else {
      src.get(id, function (err, sourceDoc) {
        if (err && err.status === 404 || (!err && (targetDoc.last_seq !== sourceDoc.last_seq))) {
          callback(null, 0);
        } else if (err) {
          callback(err);
        } else {
          callback(null, sourceDoc.last_seq);
        }
      });
    }
  });
}

function writeCheckpoint(src, target, id, checkpoint, callback) {
  function updateCheckpoint(db, callback) {
    db.get(id, function (err, doc) {
      if (err && err.status === 404) {
        doc = {_id: id};
      }
      doc.last_seq = checkpoint;
      db.put(doc, callback);
    });
  }
  updateCheckpoint(target, function (err, doc) {
    updateCheckpoint(src, function (err, doc) {
      callback();
    });
  });
}

function replicate(src, target, opts, promise) {

  var requests = new RequestManager(promise);
  var writeQueue = [];
  var repId = genReplicationId(src, target, opts);
  var results = [];
  var completed = false;
  var pendingRevs = 0;
  var last_seq = 0;
  var continuous = opts.continuous || false;
  var doc_ids = opts.doc_ids;
  var result = {
    ok: true,
    start_time: new Date(),
    docs_read: 0,
    docs_written: 0
  };

  function docsWritten(err, res, len) {
    if (opts.onChange) {
      for (var i = 0; i < len; i++) {
        /*jshint validthis:true */
        opts.onChange.apply(this, [result]);
      }
    }
    pendingRevs -= len;
    result.docs_written += len;

    writeCheckpoint(src, target, repId, last_seq, function (err, res) {
      requests.notifyRequestComplete();
      isCompleted();
    });
  }

  function writeDocs() {
    if (!writeQueue.length) {
      return requests.notifyRequestComplete();
    }
    var len = writeQueue.length;
    target.bulkDocs({docs: writeQueue}, {new_edits: false}, function (err, res) {
      docsWritten(err, res, len);
    });
    writeQueue = [];
  }

  function eachRev(id, rev) {
    src.get(id, {revs: true, rev: rev, attachments: true}, function (err, doc) {
      result.docs_read++;
      requests.notifyRequestComplete();
      writeQueue.push(doc);
      requests.enqueue(writeDocs);
    });
  }

  function onRevsDiff(diffCounts) {
    return function (err, diffs) {
      requests.notifyRequestComplete();
      if (err) {
        if (continuous) {
          promise.cancel();
        }
        PouchUtils.call(opts.complete, err, null);
        return;
      }

      // We already have all diffs passed in `diffCounts`
      if (Object.keys(diffs).length === 0) {
        for (var docid in diffCounts) {
          pendingRevs -= diffCounts[docid];
        }
        isCompleted();
        return;
      }

      var _enqueuer = function (rev) {
        requests.enqueue(eachRev, [id, rev]);
      };

      for (var id in diffs) {
        var diffsAlreadyHere = diffCounts[id] - diffs[id].missing.length;
        pendingRevs -= diffsAlreadyHere;
        diffs[id].missing.forEach(_enqueuer);
      }
    };
  }

  function fetchRevsDiff(diff, diffCounts) {
    target.revsDiff(diff, onRevsDiff(diffCounts));
  }

  function onChange(change) {
    last_seq = change.seq;
    results.push(change);
    var diff = {};
    diff[change.id] = change.changes.map(function (x) { return x.rev; });
    var counts = {};
    counts[change.id] = change.changes.length;
    pendingRevs += change.changes.length;
    requests.enqueue(fetchRevsDiff, [diff, counts]);
  }

  function complete() {
    completed = true;
    isCompleted();
  }

  function isCompleted() {
    if (completed && pendingRevs === 0) {
      result.end_time = new Date();
      PouchUtils.call(opts.complete, null, result);
    }
  }

  fetchCheckpoint(src, target, repId, function (err, checkpoint) {

    if (err) {
      return PouchUtils.call(opts.complete, err);
    }

    last_seq = checkpoint;

    // Was the replication cancelled by the caller before it had a chance
    // to start. Shouldnt we be calling complete?
    if (promise.cancelled) {
      return;
    }

    var repOpts = {
      continuous: continuous,
      since: last_seq,
      style: 'all_docs',
      onChange: onChange,
      complete: complete,
      doc_ids: doc_ids
    };

    if (opts.filter) {
      repOpts.filter = opts.filter;
    }

    if (opts.query_params) {
      repOpts.query_params = opts.query_params;
    }

    var changes = src.changes(repOpts);

    if (opts.continuous) {
      var cancel = promise.cancel;
      promise.cancel = function () {
        cancel();
        changes.cancel();
      };
    }
  });

}

function toPouch(db, callback) {
  if (typeof db === 'string') {
    return new Pouch(db, callback);
  }
  callback(null, db);
}

exports.replicate = function (src, target, opts, callback) {
  if (opts instanceof Function) {
    callback = opts;
    opts = {};
  }
  if (opts === undefined) {
    opts = {};
  }
  if (!opts.complete) {
    opts.complete = callback;
  }
  var replicateRet = new Promise();
  toPouch(src, function (err, src) {
    if (err) {
      return PouchUtils.call(callback, err);
    }
    toPouch(target, function (err, target) {
      if (err) {
        return PouchUtils.call(callback, err);
      }
      if (opts.server) {
        if (typeof src.replicateOnServer !== 'function') {
          return PouchUtils.call(callback, { error: 'Server replication not supported for ' + src.type() + ' adapter' });
        }
        if (src.type() !== target.type()) {
          return PouchUtils.call(callback, { error: 'Server replication for different adapter types (' + src.type() + ' and ' + target.type() + ') is not supported' });
        }
        src.replicateOnServer(target, opts, replicateRet);
      } else {
        replicate(src, target, opts, replicateRet);
      }
    });
  });
  return replicateRet;
};

},{"./index":12,"./utils":16,"__browserify_process":19}],15:[function(_dereq_,module,exports){
"use strict";

var PouchDB = _dereq_("./constructor");

PouchDB.adapters = {};
PouchDB.plugins = {};

PouchDB.prefix = '_pouch_';

PouchDB.parseAdapter = function (name) {
  var match = name.match(/([a-z\-]*):\/\/(.*)/);
  var adapter;
  if (match) {
    // the http adapter expects the fully qualified name
    name = /http(s?)/.test(match[1]) ? match[1] + '://' + match[2] : match[2];
    adapter = match[1];
    if (!PouchDB.adapters[adapter].valid()) {
      throw 'Invalid adapter';
    }
    return {name: name, adapter: match[1]};
  }

  var preferredAdapters = ['idb', 'leveldb', 'websql'];
  for (var i = 0; i < preferredAdapters.length; ++i) {
    if (preferredAdapters[i] in PouchDB.adapters) {
      adapter = PouchDB.adapters[preferredAdapters[i]];
      var use_prefix = 'use_prefix' in adapter ? adapter.use_prefix : true;

      return {
        name: use_prefix ? PouchDB.prefix + name : name,
        adapter: preferredAdapters[i]
      };
    }
  }

  throw 'No valid adapter found';
};

PouchDB.destroy = function (name, opts, callback) {
  if (typeof opts === 'function' || typeof opts === 'undefined') {
    callback = opts;
    opts = {};
  }

  if (typeof name === 'object') {
    opts = name;
    name = undefined;
  }

  if (typeof callback === 'undefined') {
    callback = function () {};
  }
  var backend = PouchDB.parseAdapter(opts.name || name);
  var dbName = backend.name;

  var cb = function (err, response) {
    if (err) {
      callback(err);
      return;
    }

    for (var plugin in PouchDB.plugins) {
      PouchDB.plugins[plugin]._delete(dbName);
    }
    //console.log(dbName + ': Delete Database');

    // call destroy method of the particular adaptor
    PouchDB.adapters[backend.adapter].destroy(dbName, opts, callback);
  };

  // remove PouchDB from allDBs
  PouchDB.removeFromAllDbs(backend, cb);
};

PouchDB.removeFromAllDbs = function (opts, callback) {
  // Only execute function if flag is enabled
  if (!PouchDB.enableAllDbs) {
    callback();
    return;
  }

  // skip http and https adaptors for allDbs
  var adapter = opts.adapter;
  if (adapter === "http" || adapter === "https") {
    callback();
    return;
  }

  // remove db from PouchDB.ALL_DBS
  new PouchDB(PouchDB.allDBName(opts.adapter), function (err, db) {
    if (err) {
      // don't fail when allDbs fail
      //console.error(err);
      callback();
      return;
    }
    // check if db has been registered in PouchDB.ALL_DBS
    var dbname = PouchDB.dbName(opts.adapter, opts.name);
    db.get(dbname, function (err, doc) {
      if (err) {
        callback();
      } else {
        db.remove(doc, function (err, response) {
          if (err) {
            //console.error(err);
          }
          callback();
        });
      }
    });
  });

};

PouchDB.adapter = function (id, obj) {
  if (obj.valid()) {
    PouchDB.adapters[id] = obj;
  }
};

PouchDB.plugin = function (id, obj) {
  PouchDB.plugins[id] = obj;
};

// flag to toggle allDbs (off by default)
PouchDB.enableAllDbs = false;

// name of database used to keep track of databases
PouchDB.ALL_DBS = "_allDbs";
PouchDB.dbName = function (adapter, name) {
  return [adapter, "-", name].join('');
};
PouchDB.realDBName = function (adapter, name) {
  return [adapter, "://", name].join('');
};
PouchDB.allDBName = function (adapter) {
  return [adapter, "://", PouchDB.prefix + PouchDB.ALL_DBS].join('');
};

PouchDB.open = function (opts, callback) {
  // Only register pouch with allDbs if flag is enabled
  if (!PouchDB.enableAllDbs) {
    callback();
    return;
  }

  var adapter = opts.adapter;
  // skip http and https adaptors for allDbs
  if (adapter === "http" || adapter === "https") {
    callback();
    return;
  }

  new PouchDB(PouchDB.allDBName(adapter), function (err, db) {
    if (err) {
      // don't fail when allDb registration fails
      //console.error(err);
      callback();
      return;
    }

    // check if db has been registered in PouchDB.ALL_DBS
    var dbname = PouchDB.dbName(adapter, opts.name);
    db.get(dbname, function (err, response) {
      if (err && err.status === 404) {
        db.put({
          _id: dbname,
          dbname: opts.originalName
        }, function (err) {
            if (err) {
              //console.error(err);
            }

            callback();
          });
      } else {
        callback();
      }
    });
  });
};

PouchDB.allDbs = function (callback) {
  var accumulate = function (adapters, all_dbs) {
    if (adapters.length === 0) {
      // remove duplicates
      var result = [];
      all_dbs.forEach(function (doc) {
        var exists = result.some(function (db) {
          return db.id === doc.id;
        });

        if (!exists) {
          result.push(doc);
        }
      });

      // return an array of dbname
      callback(null, result.map(function (row) {
          return row.doc.dbname;
        }));
      return;
    }

    var adapter = adapters.shift();

    // skip http and https adaptors for allDbs
    if (adapter === "http" || adapter === "https") {
      accumulate(adapters, all_dbs);
      return;
    }

    new PouchDB(PouchDB.allDBName(adapter), function (err, db) {
      if (err) {
        callback(err);
        return;
      }
      db.allDocs({include_docs: true}, function (err, response) {
        if (err) {
          callback(err);
          return;
        }

        // append from current adapter rows
        all_dbs.unshift.apply(all_dbs, response.rows);

        // code to clear allDbs.
        // response.rows.forEach(function (row) {
        //   db.remove(row.doc, function () {
        //     //console.log(arguments);
        //   });
        // });

        // recurse
        accumulate(adapters, all_dbs);
      });
    });
  };
  var adapters = Object.keys(PouchDB.adapters);
  accumulate(adapters, []);
};

module.exports = PouchDB;

},{"./constructor":5}],16:[function(_dereq_,module,exports){
/*jshint strict: false */
/*global chrome */

var merge = _dereq_('./merge');
exports.extend = _dereq_('./deps/extend');
exports.ajax = _dereq_('./deps/ajax');
exports.createBlob = _dereq_('./deps/blob');
var uuid = _dereq_('./deps/uuid');
exports.Crypto = _dereq_('./deps/md5.js');
var buffer = _dereq_('./deps/buffer');
var errors = _dereq_('./deps/errors');

// List of top level reserved words for doc
var reservedWords = [
  '_id',
  '_rev',
  '_attachments',
  '_deleted',
  '_revisions',
  '_revs_info',
  '_conflicts',
  '_deleted_conflicts',
  '_local_seq',
  '_rev_tree'
];
exports.uuids = function (count, options) {

  if (typeof(options) !== 'object') {
    options = {};
  }

  var length = options.length;
  var radix = options.radix;
  var uuids = [];

  while (uuids.push(uuid(length, radix)) < count) { }

  return uuids;
};

// Give back one UUID
exports.uuid = function (options) {
  return exports.uuids(1, options)[0];
};
// Determine id an ID is valid
//   - invalid IDs begin with an underescore that does not begin '_design' or '_local'
//   - any other string value is a valid id
exports.isValidId = function (id) {
  if (!id || (typeof id !== 'string')) {
    return false;
  }
  if (/^_/.test(id)) {
    return (/^_(design|local)/).test(id);
  }
  return true;
};

function isChromeApp() {
  return (typeof chrome !== "undefined" &&
          typeof chrome.storage !== "undefined" &&
          typeof chrome.storage.local !== "undefined");
}

// Pretty dumb name for a function, just wraps callback calls so we dont
// to if (callback) callback() everywhere
exports.call = function (fun) {
  if (typeof fun === typeof Function) {
    var args = Array.prototype.slice.call(arguments, 1);
    fun.apply(this, args);
  }
};

exports.isLocalId = function (id) {
  return (/^_local/).test(id);
};

// check if a specific revision of a doc has been deleted
//  - metadata: the metadata object from the doc store
//  - rev: (optional) the revision to check. defaults to winning revision
exports.isDeleted = function (metadata, rev) {
  if (!rev) {
    rev = merge.winningRev(metadata);
  }
  if (rev.indexOf('-') >= 0) {
    rev = rev.split('-')[1];
  }
  var deleted = false;
  merge.traverseRevTree(metadata.rev_tree, function (isLeaf, pos, id, acc, opts) {
    if (id === rev) {
      deleted = !!opts.deleted;
    }
  });

  return deleted;
};

exports.filterChange = function (opts) {
  return function (change) {
    var req = {};
    var hasFilter = opts.filter && typeof opts.filter === 'function';

    req.query = opts.query_params;
    if (opts.filter && hasFilter && !opts.filter.call(this, change.doc, req)) {
      return false;
    }
    if (opts.doc_ids && opts.doc_ids.indexOf(change.id) === -1) {
      return false;
    }
    if (!opts.include_docs) {
      delete change.doc;
    } else {
      for (var att in change.doc._attachments) {
        change.doc._attachments[att].stub = true;
      }
    }
    return true;
  };
};

exports.processChanges = function (opts, changes, last_seq) {
  // TODO: we should try to filter and limit as soon as possible
  changes = changes.filter(exports.filterChange(opts));
  if (opts.limit) {
    if (opts.limit < changes.length) {
      changes.length = opts.limit;
    }
  }
  changes.forEach(function (change) {
    exports.call(opts.onChange, change);
  });
  exports.call(opts.complete, null, {results: changes, last_seq: last_seq});
};

// Preprocess documents, parse their revisions, assign an id and a
// revision for new writes that are missing them, etc
exports.parseDoc = function (doc, newEdits) {
  var error = null;
  var nRevNum;
  var newRevId;
  var revInfo;
  var opts = {status: 'available'};
  if (doc._deleted) {
    opts.deleted = true;
  }

  if (newEdits) {
    if (!doc._id) {
      doc._id = exports.uuid();
    }
    newRevId = exports.uuid({length: 32, radix: 16}).toLowerCase();
    if (doc._rev) {
      revInfo = /^(\d+)-(.+)$/.exec(doc._rev);
      if (!revInfo) {
        throw "invalid value for property '_rev'";
      }
      doc._rev_tree = [{
        pos: parseInt(revInfo[1], 10),
        ids: [revInfo[2], {status: 'missing'}, [[newRevId, opts, []]]]
      }];
      nRevNum = parseInt(revInfo[1], 10) + 1;
    } else {
      doc._rev_tree = [{
        pos: 1,
        ids : [newRevId, opts, []]
      }];
      nRevNum = 1;
    }
  } else {
    if (doc._revisions) {
      doc._rev_tree = [{
        pos: doc._revisions.start - doc._revisions.ids.length + 1,
        ids: doc._revisions.ids.reduce(function (acc, x) {
          if (acc === null) {
            return [x, opts, []];
          } else {
            return [x, {status: 'missing'}, [acc]];
          }
        }, null)
      }];
      nRevNum = doc._revisions.start;
      newRevId = doc._revisions.ids[0];
    }
    if (!doc._rev_tree) {
      revInfo = /^(\d+)-(.+)$/.exec(doc._rev);
      if (!revInfo) {
        return errors.BAD_ARG;
      }
      nRevNum = parseInt(revInfo[1], 10);
      newRevId = revInfo[2];
      doc._rev_tree = [{
        pos: parseInt(revInfo[1], 10),
        ids: [revInfo[2], opts, []]
      }];
    }
  }

  if (typeof doc._id !== 'string') {
    error = errors.INVALID_ID;
  }
  else if (!exports.isValidId(doc._id)) {
    error = errors.RESERVED_ID;
  }

  for (var key in doc) {
    if (doc.hasOwnProperty(key) && key[0] === '_' && reservedWords.indexOf(key) === -1) {
      error = exports.extend({}, errors.DOC_VALIDATION);
      error.reason += ': ' + key;
    }
  }

  doc._id = decodeURIComponent(doc._id);
  doc._rev = [nRevNum, newRevId].join('-');

  if (error) {
    return error;
  }

  return Object.keys(doc).reduce(function (acc, key) {
    if (/^_/.test(key) && key !== '_attachments') {
      acc.metadata[key.slice(1)] = doc[key];
    } else {
      acc.data[key] = doc[key];
    }
    return acc;
  }, {metadata : {}, data : {}});
};

exports.isCordova = function () {
  return (typeof cordova !== "undefined" ||
          typeof PhoneGap !== "undefined" ||
          typeof phonegap !== "undefined");
};

exports.Changes = function () {

  var api = {};
  var listeners = {};

  if (isChromeApp()) {
    chrome.storage.onChanged.addListener(function (e) {
      // make sure it's event addressed to us
      if (e.db_name != null) {
        api.notify(e.db_name.newValue);//object only has oldValue, newValue members
      }
    });
  } else if (typeof window !== 'undefined') {
    window.addEventListener("storage", function (e) {
      api.notify(e.key);
    });
  }

  api.addListener = function (db_name, id, db, opts) {
    if (!listeners[db_name]) {
      listeners[db_name] = {};
    }
    listeners[db_name][id] = {
      db: db,
      opts: opts
    };
  };

  api.removeListener = function (db_name, id) {
    if (listeners[db_name]) {
      delete listeners[db_name][id];
    }
  };

  api.clearListeners = function (db_name) {
    delete listeners[db_name];
  };

  api.notifyLocalWindows = function (db_name) {
    //do a useless change on a storage thing
    //in order to get other windows's listeners to activate
    if (!isChromeApp()) {
      localStorage[db_name] = (localStorage[db_name] === "a") ? "b" : "a";
    } else {
      chrome.storage.local.set({db_name: db_name});
    }
  };

  api.notify = function (db_name) {
    if (!listeners[db_name]) { return; }

    Object.keys(listeners[db_name]).forEach(function (i) {
      var opts = listeners[db_name][i].opts;
      listeners[db_name][i].db.changes({
        include_docs: opts.include_docs,
        conflicts: opts.conflicts,
        continuous: false,
        descending: false,
        filter: opts.filter,
        view: opts.view,
        since: opts.since,
        query_params: opts.query_params,
        onChange: function (c) {
          if (c.seq > opts.since && !opts.cancelled) {
            opts.since = c.seq;
            exports.call(opts.onChange, c);
          }
        }
      });
    });
  };

  return api;
};

if (typeof window === 'undefined' || !('atob' in window)) {
  exports.atob = function (str) {
    var base64 = new buffer(str, 'base64');
    // Node.js will just skip the characters it can't encode instead of
    // throwing and exception
    if (base64.toString('base64') !== str) {
      throw ("Cannot base64 encode full string");
    }
    return base64.toString('binary');
  };
} else {
  exports.atob = function (str) {
    return atob(str);
  };
}

if (typeof window === 'undefined' || !('btoa' in window)) {
  exports.btoa = function (str) {
    return new buffer(str, 'binary').toString('base64');
  };
} else {
  exports.btoa = function (str) {
    return btoa(str);
  };
}


module.exports = exports;

},{"./deps/ajax":6,"./deps/blob":7,"./deps/buffer":18,"./deps/errors":8,"./deps/extend":9,"./deps/md5.js":10,"./deps/uuid":11,"./merge":13}],17:[function(_dereq_,module,exports){
module.exports = '1.1.0';

},{}],18:[function(_dereq_,module,exports){

},{}],19:[function(_dereq_,module,exports){
// shim for using process in browser

var process = module.exports = {};

process.nextTick = (function () {
    var canSetImmediate = typeof window !== 'undefined'
    && window.setImmediate;
    var canPost = typeof window !== 'undefined'
    && window.postMessage && window.addEventListener
    ;

    if (canSetImmediate) {
        return function (f) { return window.setImmediate(f) };
    }

    if (canPost) {
        var queue = [];
        window.addEventListener('message', function (ev) {
            var source = ev.source;
            if ((source === window || source === null) && ev.data === 'process-tick') {
                ev.stopPropagation();
                if (queue.length > 0) {
                    var fn = queue.shift();
                    fn();
                }
            }
        }, true);

        return function nextTick(fn) {
            queue.push(fn);
            window.postMessage('process-tick', '*');
        };
    }

    return function nextTick(fn) {
        setTimeout(fn, 0);
    };
})();

process.title = 'browser';
process.browser = true;
process.env = {};
process.argv = [];

process.binding = function (name) {
    throw new Error('process.binding is not supported');
}

// TODO(shtylman)
process.cwd = function () { return '/' };
process.chdir = function (dir) {
    throw new Error('process.chdir is not supported');
};

},{}],20:[function(_dereq_,module,exports){
/*global Pouch: true, pouchCollate: true */

"use strict";

var pouchCollate = _dereq_('pouchdb-collate');

// This is the first implementation of a basic plugin, we register the
// plugin object with pouch and it is mixin'd to each database created
// (regardless of adapter), adapters can override plugins by providing
// their own implementation. functions on the plugin object that start
// with _ are reserved function that are called by pouchdb for special
// notifications.

// If we wanted to store incremental views we can do it here by listening
// to the changes feed (keeping track of our last update_seq between page loads)
// and storing the result of the map function (possibly using the upcoming
// extracted adapter functions)



function MapReduce(db) {
  if(!(this instanceof MapReduce)){
    return new MapReduce(db);
  }
  function viewQuery(fun, options) {
    if (!options.complete) {
      return;
    }

    if (!options.skip) {
      options.skip = 0;
    }

    if (!fun.reduce) {
      options.reduce = false;
    }

    function sum(values) {
      return values.reduce(function (a, b) { return a + b; }, 0);
    }

    var builtInReduce = {
      "_sum": function (keys, values){
        return sum(values);
      },

      "_count": function (keys, values, rereduce){
        if (rereduce){
          return sum(values);
        } else {
          return values.length;
        }
      },

      "_stats": function (keys, values, rereduce) {
        return {
          'sum': sum(values),
          'min': Math.min.apply(null, values),
          'max': Math.max.apply(null, values),
          'count': values.length,
          'sumsqr': (function () {
            var _sumsqr = 0;
            for(var idx in values) {
              if (typeof values[idx] === 'number') {
              _sumsqr += values[idx] * values[idx];
              }
            }
            return _sumsqr;
          })()
        };
      }
    };

    var results = [];
    var current = null;
    var num_started= 0;
    var completed= false;

    function emit(key, val) {
      var viewRow = {
        id: current.doc._id,
        key: key,
        value: val
      };

      if (options.startkey && pouchCollate(key, options.startkey) < 0) return;
      if (options.endkey && pouchCollate(key, options.endkey) > 0) return;
      if (typeof options.key !== 'undefined' && pouchCollate(key, options.key) !== 0) return;

      num_started++;
      if (options.include_docs) {
        //in this special case, join on _id (issue #106)
        if (val && typeof val === 'object' && val._id){
          db.get(val._id,
              function (_, joined_doc){
                if (joined_doc) {
                  viewRow.doc = joined_doc;
                }
                results.push(viewRow);
                checkComplete();
              });
          return;
        } else {
          viewRow.doc = current.doc;
        }
      }
      results.push(viewRow);
    };

    // ugly way to make sure references to 'emit' in map/reduce bind to the
    // above emit
    eval('fun.map = ' + fun.map.toString() + ';');
    if (fun.reduce) {
      if (builtInReduce[fun.reduce]) {
        fun.reduce = builtInReduce[fun.reduce];
      }

      eval('fun.reduce = ' + fun.reduce.toString() + ';');
    }

    //only proceed once all documents are mapped and joined
    function checkComplete() {
      if (completed && results.length == num_started){
        results.sort(function (a, b) {
          return pouchCollate(a.key, b.key);
        });
        if (options.descending) {
          results.reverse();
        }
        if (options.reduce === false) {
          return options.complete(null, {
            total_rows: results.length,
            offset: options.skip,
            rows: ('limit' in options) ? results.slice(options.skip, options.limit + options.skip) :
              (options.skip > 0) ? results.slice(options.skip) : results
          });
        }

        var groups = [];
        results.forEach(function (e) {
          var last = groups[groups.length-1] || null;
          if (last && pouchCollate(last.key[0][0], e.key) === 0) {
            last.key.push([e.key, e.id]);
            last.value.push(e.value);
            return;
          }
          groups.push({key: [[e.key, e.id]], value: [e.value]});
        });
        groups.forEach(function (e) {
          e.value = fun.reduce(e.key, e.value);
          e.value = (typeof e.value === 'undefined') ? null : e.value;
          e.key = e.key[0][0];
        });

        options.complete(null, {
          total_rows: groups.length,
          offset: options.skip,
          rows: ('limit' in options) ? groups.slice(options.skip, options.limit + options.skip) :
            (options.skip > 0) ? groups.slice(options.skip) : groups
        });
      }
    };

    db.changes({
      conflicts: true,
      include_docs: true,
      onChange: function (doc) {
        if (!('deleted' in doc)) {
          current = {doc: doc.doc};
          fun.map.call(this, doc.doc);
        }
      },
      complete: function () {
        completed= true;
        checkComplete();
      }
    });
  }

  function httpQuery(fun, opts, callback) {

    // List of parameters to add to the PUT request
    var params = [];
    var body = undefined;
    var method = 'GET';

    // If opts.reduce exists and is defined, then add it to the list
    // of parameters.
    // If reduce=false then the results are that of only the map function
    // not the final result of map and reduce.
    if (typeof opts.reduce !== 'undefined') {
      params.push('reduce=' + opts.reduce);
    }
    if (typeof opts.include_docs !== 'undefined') {
      params.push('include_docs=' + opts.include_docs);
    }
    if (typeof opts.limit !== 'undefined') {
      params.push('limit=' + opts.limit);
    }
    if (typeof opts.descending !== 'undefined') {
      params.push('descending=' + opts.descending);
    }
    if (typeof opts.startkey !== 'undefined') {
      params.push('startkey=' + encodeURIComponent(JSON.stringify(opts.startkey)));
    }
    if (typeof opts.endkey !== 'undefined') {
      params.push('endkey=' + encodeURIComponent(JSON.stringify(opts.endkey)));
    }
    if (typeof opts.key !== 'undefined') {
      params.push('key=' + encodeURIComponent(JSON.stringify(opts.key)));
    }
    if (typeof opts.group !== 'undefined') {
      params.push('group=' + opts.group);
    }
    if (typeof opts.group_level !== 'undefined') {
      params.push('group_level=' + opts.group_level);
    }
    if (typeof opts.skip !== 'undefined') {
      params.push('skip=' + opts.skip);
    }

    // If keys are supplied, issue a POST request to circumvent GET query string limits
    // see http://wiki.apache.org/couchdb/HTTP_view_API#Querying_Options
    if (typeof opts.keys !== 'undefined') {
      method = 'POST';
      body = JSON.stringify({keys:opts.keys});
    }

    // Format the list of parameters into a valid URI query string
    params = params.join('&');
    params = params === '' ? '' : '?' + params;

    // We are referencing a query defined in the design doc
    if (typeof fun === 'string') {
      var parts = fun.split('/');
      db.request({
        method: method,
        url: '_design/' + parts[0] + '/_view/' + parts[1] + params,
        body: body
      }, callback);
      return;
    }

    // We are using a temporary view, terrible for performance but good for testing
    var queryObject = JSON.parse(JSON.stringify(fun, function (key, val) {
      if (typeof val === 'function') {
        return val + ''; // implicitly `toString` it
      }
      return val;
    }));

    db.request({
      method:'POST',
      url: '_temp_view' + params,
      body: queryObject
    }, callback);
  }

  this.query = function(fun, opts, callback) {
    if (typeof opts === 'function') {
      callback = opts;
      opts = {};
    }

    if (callback) {
      opts.complete = callback;
    }

    if (db.type() === 'http') {
    if (typeof fun === 'function'){
      return httpQuery({map: fun}, opts, callback);
    }
    return httpQuery(fun, opts, callback);
    }

    if (typeof fun === 'object') {
      return viewQuery(fun, opts);
    }

    if (typeof fun === 'function') {
      return viewQuery({map: fun}, opts);
    }

    var parts = fun.split('/');
    db.get('_design/' + parts[0], function (err, doc) {
      if (err) {
        if (callback) callback(err);
        return;
      }

      if (!doc.views[parts[1]]) {
        if (callback) callback({ error: 'not_found', reason: 'missing_named_view' });
        return;
      }

      viewQuery({
        map: doc.views[parts[1]].map,
        reduce: doc.views[parts[1]].reduce
      }, opts);
    });
  }

};

// Deletion is a noop since we dont store the results of the view
MapReduce._delete = function () { };
module.exports = MapReduce;

},{"pouchdb-collate":21}],21:[function(_dereq_,module,exports){
'use strict';

function arrayCollate(a, b) {
  var len = Math.min(a.length, b.length);
  for (var i = 0; i < len; i++) {
    var sort = pouchCollate(a[i], b[i]);
    if (sort !== 0) {
      return sort;
    }
  }
  return (a.length === b.length) ? 0 :
    (a.length > b.length) ? 1 : -1;
}
function stringCollate(a, b) {
  // See: https://github.com/daleharvey/pouchdb/issues/40
  // This is incompatible with the CouchDB implementation, but its the
  // best we can do for now
  return (a === b) ? 0 : ((a > b) ? 1 : -1);
}
function objectCollate(a, b) {
  var ak = Object.keys(a), bk = Object.keys(b);
  var len = Math.min(ak.length, bk.length);
  for (var i = 0; i < len; i++) {
    // First sort the keys
    var sort = pouchCollate(ak[i], bk[i]);
    if (sort !== 0) {
      return sort;
    }
    // if the keys are equal sort the values
    sort = pouchCollate(a[ak[i]], b[bk[i]]);
    if (sort !== 0) {
      return sort;
    }

  }
  return (ak.length === bk.length) ? 0 :
    (ak.length > bk.length) ? 1 : -1;
}
// The collation is defined by erlangs ordered terms
// the atoms null, true, false come first, then numbers, strings,
// arrays, then objects
function collationIndex(x) {
  var id = ['boolean', 'number', 'string', 'object'];
  if (id.indexOf(typeof x) !== -1) {
    if (x === null) {
      return 1;
    }
    return id.indexOf(typeof x) + 2;
  }
  if (Array.isArray(x)) {
    return 4.5;
  }
  if (typeof x === 'undefined') {
    // CouchDB indexes both null/undefined as null
    return 1;
  }
}
module.exports = pouchCollate;
function pouchCollate(a, b) {
  var ai = collationIndex(a);
  var bi = collationIndex(b);
  if ((ai - bi) !== 0) {
    return ai - bi;
  }
  if (a === null || typeof a === 'undefined') {
    return 0;
  }
  if (typeof a === 'number') {
    return a - b;
  }
  if (typeof a === 'boolean') {
    return a === b ? 0 : (a < b ? -1 : 1);
  }
  if (typeof a === 'string') {
    return stringCollate(a, b);
  }
  if (Array.isArray(a)) {
    return arrayCollate(a, b);
  }
  if (typeof a === 'object') {
    return objectCollate(a, b);
  }
};;
},{}]},{},[12])
(12)
});