// Copyright 2011 The Closure Library Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Wrapper for an IndexedDB transaction.
 *
 */


goog.provide('goog.db.Transaction');
goog.provide('goog.db.Transaction.TransactionMode');

goog.require('goog.db.Error');
goog.require('goog.db.ObjectStore');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');



/**
 * Creates a new transaction. Transactions contain methods for accessing object
 * stores and are created from the database object. Should not be created
 * directly, open a database and call createTransaction on it.
 * @see goog.db.IndexedDb#createTransaction
 *
 * @param {!IDBTransaction} tx IndexedDB transaction to back this wrapper.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
goog.db.Transaction = function(tx) {
  goog.base(this);

  /**
   * Underlying IndexedDB transaction object.
   *
   * @type {!IDBTransaction}
   * @private
   */
  this.tx_ = tx;

  /**
   * Event handler for this transaction.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  // TODO(user): remove these casts once the externs file is updated to
  // correctly reflect that IDBTransaction extends EventTarget
  this.eventHandler_.listen(
      (/** @type {EventTarget} */ this.tx_),
      'complete',
      goog.bind(
          this.dispatchEvent,
          this,
          goog.db.Transaction.EventTypes.COMPLETE));
  this.eventHandler_.listen(
      (/** @type {EventTarget} */ this.tx_),
      'abort',
      goog.bind(
          this.dispatchEvent,
          this,
          goog.db.Transaction.EventTypes.ABORT));
  this.eventHandler_.listen(
      (/** @type {EventTarget} */ this.tx_),
      'error',
      this.dispatchError_);
};
goog.inherits(goog.db.Transaction, goog.events.EventTarget);


/**
 * Dispatches an error event based on the given event, wrapping the error
 * if necessary.
 *
 * @param {Event} ev The error event given to the underlying IDBTransaction.
 * @private
 */
goog.db.Transaction.prototype.dispatchError_ = function(ev) {
  if (ev.target instanceof goog.db.Error) {
    this.dispatchEvent({
      type: goog.db.Transaction.EventTypes.ERROR,
      target: ev.target
    });
  } else {
    this.dispatchEvent({
      type: goog.db.Transaction.EventTypes.ERROR,
      target: new goog.db.Error(
          (/** @type {IDBRequest} */ (ev.target)).errorCode,
          'in transaction')
    });
  }
};


/**
 * Event types the Transaction can dispatch. COMPLETE events are dispatched
 * when the transaction is committed. If a transaction is aborted it dispatches
 * both an ABORT event and an ERROR event with the ABORT_ERR code. Error events
 * are dispatched on any error.
 *
 * @enum {string}
 */
goog.db.Transaction.EventTypes = {
  COMPLETE: 'complete',
  ABORT: 'abort',
  ERROR: 'error'
};


/**
 * @return {goog.db.Transaction.TransactionMode} The transaction's mode.
 */
goog.db.Transaction.prototype.getMode = function() {
  return /** @type {goog.db.Transaction.TransactionMode} */ (this.tx_.mode);
};


/**
 * Opens an object store to do operations on in this transaction. The requested
 * object store must be one that is in this transaction's scope.
 * @see goog.db.IndexedDb#createTransaction
 *
 * @param {string} name The name of the requested object store.
 * @return {!goog.db.ObjectStore} The wrapped object store.
 * @throws {goog.db.Error} In case of error getting the object store.
 */
goog.db.Transaction.prototype.objectStore = function(name) {
  try {
    return new goog.db.ObjectStore(this.tx_.objectStore(name));
  } catch (err) {
    throw new goog.db.Error(err.code, 'getting object store ' + name);
  }
};


/**
 * Aborts this transaction. No pending operations will be applied to the
 * database. Dispatches an ABORT event.
 */
goog.db.Transaction.prototype.abort = function() {
  this.tx_.abort();
};


/** @override */
goog.db.Transaction.prototype.disposeInternal = function() {
  goog.base(this, 'disposeInternal');
  this.eventHandler_.dispose();
};


/**
 * The three possible transaction modes.
 * @see http://www.w3.org/TR/IndexedDB/#idl-def-IDBTransaction
 *
 * @enum {number}
 */
goog.db.Transaction.TransactionMode = {
  READ_ONLY: 0,
  READ_WRITE: 1,
  VERSION_CHANGE: 2
};
