// Copyright 2025 RethinkDB, all rights reserved.
// V8 JavaScript engine implementation header

#ifndef RETHINKDB_EXTPROC_JS_ENGINE_V8_HPP_
#define RETHINKDB_EXTPROC_JS_ENGINE_V8_HPP_

#include <memory>

#include "extproc/js_engine.hpp"

namespace rethinkdb {
namespace js {

// Create V8 jitless engine instance
std::unique_ptr<js_engine_t> create_v8_jitless_engine();

// Create V8 full engine instance (with JIT)
std::unique_ptr<js_engine_t> create_v8_full_engine();

}  // namespace js
}  // namespace rethinkdb

#endif  // RETHINKDB_EXTPROC_JS_ENGINE_V8_HPP_
