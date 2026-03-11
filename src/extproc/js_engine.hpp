// Copyright 2025 RethinkDB, all rights reserved.
// JavaScript engine abstraction interface for r.js()

#ifndef RETHINKDB_EXTPROC_JS_ENGINE_HPP_
#define RETHINKDB_EXTPROC_JS_ENGINE_HPP_

#include <string>
#include <memory>
#include <vector>

#include "containers/archive/archive.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/error.hpp"

namespace rethinkdb {
namespace js {

// JavaScript engine types supported by RethinkDB
enum class js_engine_type_t {
    V8_JITLESS,     // V8 in jitless mode (default - secure)
    V8_FULL,        // V8 with JIT compilation
    QUICKJS,        // QuickJS (small footprint)
    QUICKJS_NG,     // QuickJS-NG fork
    DUKTAPE,        // Duktape (ultra-small)
    HERMES          // Hermes (mobile/embedded)
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    js_engine_type_t, int8_t,
    static_cast<int8_t>(js_engine_type_t::V8_JITLESS),
    static_cast<int8_t>(js_engine_type_t::HERMES));

// Abstract JavaScript engine interface
// This interface abstracts away the underlying JS engine implementation,
// allowing RethinkDB to support multiple engines (V8, QuickJS, etc.)
// with a common interface.
class js_engine_t {
public:
    virtual ~js_engine_t() = default;

    // Evaluate JavaScript code and return result as datum
    // Throws ql::base_exc_t on error
    virtual ql::datum_t eval(const std::string& code,
                             const ql::configured_limits_t& limits) = 0;

    // Call a JavaScript function by name with arguments
    // The function must exist in the global scope
    virtual ql::datum_t call(const std::string& func_name,
                             const std::vector<ql::datum_t>& args,
                             const ql::configured_limits_t& limits) = 0;

    // Set a global variable in the JS environment
    virtual void set_global(const std::string& name, const ql::datum_t& value) = 0;

    // Get a global variable from the JS environment
    virtual ql::datum_t get_global(const std::string& name,
                                   const ql::configured_limits_t& limits) = 0;

    // Get current memory usage in bytes
    virtual size_t get_memory_usage() const = 0;

    // Request garbage collection (hint only)
    virtual void gc() = 0;

    // Get the engine type
    virtual js_engine_type_t get_type() const = 0;

    // Get engine version string
    virtual std::string get_version() const = 0;

protected:
    js_engine_t() = default;
    DISABLE_COPYING(js_engine_t);
};

// Factory function to create an engine instance
std::unique_ptr<js_engine_t> create_js_engine(js_engine_type_t type);

// Get default engine type based on build configuration
js_engine_type_t get_default_js_engine();

// Parse engine name to type
js_engine_type_t parse_js_engine_name(const std::string& name);

// Get string representation of engine type
std::string get_js_engine_name(js_engine_type_t type);

// Engine-specific factory functions (may return nullptr if not compiled in)
std::unique_ptr<js_engine_t> create_v8_jitless_engine();
std::unique_ptr<js_engine_t> create_v8_full_engine();
std::unique_ptr<js_engine_t> create_quickjs_engine();
std::unique_ptr<js_engine_t> create_quickjs_ng_engine();
std::unique_ptr<js_engine_t> create_duktape_engine();
std::unique_ptr<js_engine_t> create_hermes_engine();

}  // namespace js
}  // namespace rethinkdb

#endif  // RETHINKDB_EXTPROC_JS_ENGINE_HPP_
