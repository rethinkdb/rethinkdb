// Copyright 2010-2012 RethinkDB, all rights reserved.
goog.provide('rethinkdb.Cursor');

/**
 * The return value of calls to run. The cursor object collects the results of queries
 * @constructor
 */
rethinkdb.Cursor = function(conn, query, token) {
    this.conn_ = conn;
    this.query_ = query; // Origional ReQL query
    this.token_ = token;

    this.partial_ = [];
    this.done_ = false;
    this.continuation_ = null;
    this.streamPaused_ = false;
};

/**
 * Indicates that we are no longer interested in any more results from this query.
 */
rethinkdb.Cursor.prototype.close = function() {
    this.conn_.forgetCursor(this.token_);
};

/**
 * Retreive the next value in the result and pass to callback.
 * Return true to continue iterating over the results. Return false
 * to stop iteration. Next may be called again later to continue
 * iteration from where it was left off. Large results will be
 * lazily loaded.
 */
rethinkdb.Cursor.prototype.next = function(callback) {
    var result = this.partial_.shift();
    if (result === undefined) {
        if (this.done_) {
            // call callback once with result to indicate end of stream
            callback(result);
        } else {
            // we need the next batch
            this.continuation_ = goog.bind(this.next, this, callback);
            this.loadMore();
        }
    } else {
        if(callback(result)) {
            this.next(callback);
        } // else stop iteration
    }
};
goog.exportProperty(rethinkdb.Cursor.prototype, 'next',
                    rethinkdb.Cursor.prototype.next);

/**
 * Buffer all results and execute callback once with the results.
 */
rethinkdb.Cursor.prototype.collect = function(callback) {
    if (this.done_) {
        callback(this.partial_.splice(0, this.partial_.length));
    } else {
        // continue loading the stream
        this.continuation_ = goog.bind(this.collect, this, callback);
        this.loadMore();
    }
};
goog.exportProperty(rethinkdb.Cursor.prototype, 'collect',
                    rethinkdb.Cursor.prototype.collect);

/**************\
|* Non public *|
|*  used by   *|
|* connection *|
\**************/

/**
 * Adds another batch of results to the cursor
 * @ignore
 */
rethinkdb.Cursor.prototype.concatResults = function(moreResults, done) {
    if (goog.isArray(moreResults)) {
        Array.prototype.push.apply(this.partial_, moreResults);
    } else {
        this.partial_.push(moreResults);
    }
    this.done_ = done;
    this.streamPaused_ = true;
    if (this.continuation_) this.continuation_();
};

/** @ignore */
rethinkdb.Cursor.prototype.loadMore = function() {
    if (this.streamPaused_) this.conn_.requestNextBatch();
    this.streamPaused_ = false;
};
