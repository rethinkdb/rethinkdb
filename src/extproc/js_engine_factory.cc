// Copyright 2025 RethinkDB, all rights reserved.
// Factory implementation for JavaScript engine creation

#include "extproc/js_engine.hpp"

#include <memory>
#include <stdexcept>

// Include engine-specific headers based on build configuration
#if defined(RETHINKDB_JS_ENGINE_V8)
#include "extproc/js_engine_v8.hpp"
#endif

#if defined(RETHINKDB_JS_ENGINE_QUICKJS)
#include "extproc/js_engine_quickjs.hpp"
#endif

#if defined(RETHINKDB_JS_ENGINE_QUICKJS_NG)
#include "extproc/js_engine_quickjs_ng.hpp"
#endif

#if defined(RETHINKDB_JS_ENGINE_DUKTAPE)
#include "extproc/js_engine_duktape.hpp"
#endif

#if defined(RETHINKDB_JS_ENGINE_HERMES)
#include "extproc/js_engine_hermes.hpp"
#endif

// Forward declarations for engine-specific implementations
namespace rethinkdb {
namespace js {

// Engine factory implementation
std::unique_ptr<js_engine_t> create_js_engine(js_engine_type_t type) {
    switch (type) {
#if defined(RETHINKDB_JS_ENGINE_V8)
        case js_engine_type_t::V8_JITLESS:
            return create_v8_jitless_engine();
            
        case js_engine_type_t::V8_FULL:
            return create_v8_full_engine();
#endif
            
#if defined(RETHINKDB_JS_ENGINE_QUICKJS)
        case js_engine_type_t::QUICKJS:
            return create_quickjs_engine();
#endif
            
#if defined(RETHINKDB_JS_ENGINE_QUICKJS_NG)
        case js_engine_type_t::QUICKJS_NG:
            return create_quickjs_ng_engine();
#endif
            
#if defined(RETHINKDB_JS_ENGINE_DUKTAPE)
        case js_engine_type_t::DUKTAPE:
            return create_duktape_engine();
#endif
            
#if defined(RETHINKDB_JS_ENGINE_HERMES)
        case js_engine_type_t::HERMES:
            return create_hermes_engine();
#endif
            
        default:
            throw std::invalid_argument("JavaScript engine not available: " + 
                std::to_string(static_cast<int>(type)));
    }
}

// Get default engine type based on build configuration
js_engine_type_t get_default_js_engine() {
#if defined(RETHINKDB_JS_ENGINE_DEFAULT)
    return RETHINKDB_JS_ENGINE_DEFAULT;
#else
    // Fallback to V8 jitless
    return js_engine_type_t::V8_JITLESS;
#endif
}

// Get engine type from string
js_engine_type_t parse_js_engine_name(const std::string& name) {
    if (name == "v8-jitless" || name == "v8jitless") {
        return js_engine_type_t::V8_JITLESS;
    } else if (name == "v8" || name == "v8-full") {
        return js_engine_type_t::V8_FULL;
    } else if (name == "quickjs" || name == "quickjs-std") {
        return js_engine_type_t::QUICKJS;
    } else if (name == "quickjs-ng") {
        return js_engine_type_t::QUICKJS_NG;
    } else if (name == "duktape") {
        return js_engine_type_t::DUKTAPE;
    } else if (name == "hermes") {
        return js_engine_type_t::HERMES;
    } else {
        throw std::invalid_argument("Unknown JavaScript engine: " + name);
    }
}

// Get string representation of engine type
std::string get_js_engine_name(js_engine_type_t type) {
    switch (type) {
        case js_engine_type_t::V8_JITLESS:
            return "v8-jitless";
        case js_engine_type_t::V8_FULL:
            return "v8";
        case js_engine_type_t::QUICKJS:
            return "quickjs";
        case js_engine_type_t::QUICKJS_NG:
            return "quickjs-ng";
        case js_engine_type_t::DUKTAPE:
            return "duktape";
        case js_engine_type_t::HERMES:
            return "hermes";
        default:
            return "unknown";
    }
}

}  // namespace js
}  // namespace rethinkdb
