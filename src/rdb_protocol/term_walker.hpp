// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TERM_WALKER_HPP_
#define RDB_PROTOCOL_TERM_WALKER_HPP_

#include "rapidjson/document.h"

namespace ql {

class backtrace_registry_t;
 
// `preprocess_term_tree(...)` walks the raw term tree provided, edits it to add
// backtraces, and checks the validity of the terms and their placement.
// Most notably:
//   r.asc() and r.desc() must be direct descendants of an r.order_by() term
//   r.now() is rewritten to a literal datum
//   objects are rewritten to MAKE_OBJ terms
//   it is enforced that each term has exactly zero or one set each of args and optargs
void preprocess_term_tree(rapidjson::Value *query_json,
                          rapidjson::Value::AllocatorType *allocator,
                          backtrace_registry_t *bt_reg);

// `preprocess_global_optarg(...)` performs the same tasks as `preprocess_term_tree`,
// except that the backtraces it attaches are all the 'root' backtrace: []
void preprocess_global_optarg(rapidjson::Value *optarg,
                              rapidjson::Value::AllocatorType *allocator);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_WALKER_HPP_
