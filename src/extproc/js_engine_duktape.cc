// Copyright 2025 RethinkDB, all rights reserved.
// Duktape JavaScript engine implementation

#include "extproc/js_engine.hpp"

#if defined(RETHINKDB_JS_ENGINE_DUKTAPE)

#include <duktape.h>

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/configured_limits.hpp"

namespace rethinkdb {
namespace js {

namespace {

// Duktape engine implementation
class duktape_engine_t : public js_engine_t {
public:
    duktape_engine_t() {
        ctx_ = duk_create_heap(nullptr, nullptr, nullptr, nullptr, 
                               fatal_error_handler);
        if (!ctx_) {
            throw std::runtime_error("Failed to create Duktape context");
        }
        
        // Set memory limit (soft limit)
        // Note: Duktape doesn't have built-in memory limits,
        // use custom allocator for strict limits
    }
    
    ~duktape_engine_t() override {
        if (ctx_) {
            duk_destroy_heap(ctx_);
        }
    }
    
    ql::datum_t eval(const std::string& code,
                     const ql::configured_limits_t& limits) override {
        duk_push_string(ctx_, code.c_str());
        
        if (duk_peval(ctx_) != 0) {
            const char* error = duk_safe_to_string(ctx_, -1);
            duk_pop(ctx_);
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                std::string("JavaScript error: ") + error);
        }
        
        ql::datum_t result = duk_to_datum(-1, limits);
        duk_pop(ctx_);
        return result;
    }
    
    ql::datum_t call(const std::string& func_name,
                     const std::vector<ql::datum_t>& args,
                     const ql::configured_limits_t& limits) override {
        duk_get_global_string(ctx_, func_name.c_str());
        
        if (!duk_is_function(ctx_, -1)) {
            duk_pop(ctx_);
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                func_name + " is not a function");
        }
        
        // Push arguments
        for (const auto& arg : args) {
            push_datum(arg);
        }
        
        duk_int_t rc = duk_pcall(ctx_, static_cast<duk_idx_t>(args.size()));
        
        if (rc != DUK_EXEC_SUCCESS) {
            const char* error = duk_safe_to_string(ctx_, -1);
            duk_pop(ctx_);
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                std::string("JavaScript call error: ") + error);
        }
        
        ql::datum_t result = duk_to_datum(-1, limits);
        duk_pop(ctx_);
        return result;
    }
    
    void set_global(const std::string& name, const ql::datum_t& value) override {
        push_datum(value);
        duk_put_global_string(ctx_, name.c_str());
    }
    
    ql::datum_t get_global(const std::string& name,
                          const ql::configured_limits_t& limits) override {
        duk_get_global_string(ctx_, name.c_str());
        ql::datum_t result = duk_to_datum(-1, limits);
        duk_pop(ctx_);
        return result;
    }
    
    size_t get_memory_usage() const override {
        // Duktape doesn't expose this directly
        // Return approximate based on stack and heap
        return duk_get_memory_functions(ctx_).alloc_func ? 
               duk_get_top(ctx_) * sizeof(void*) : 0;
    }
    
    void gc() override {
        duk_gc(ctx_, 0);
    }
    
    js_engine_type_t get_type() const override {
        return js_engine_type_t::DUKTAPE;
    }
    
    std::string get_version() const override {
        return DUK_VERSION_STRING;
    }

private:
    duk_context* ctx_ = nullptr;
    
    static void fatal_error_handler(void* udata, const char* msg) {
        (void)udata;
        throw std::runtime_error(std::string("Duktape fatal error: ") + msg);
    }
    
    // Push datum onto Duktape stack
    void push_datum(const ql::datum_t& datum) {
        switch (datum.get_type()) {
            case ql::datum_t::type_t::R_NULL:
                duk_push_null(ctx_);
                break;
                
            case ql::datum_t::type_t::R_BOOL:
                duk_push_boolean(ctx_, datum.as_bool());
                break;
                
            case ql::datum_t::type_t::R_NUM:
                duk_push_number(ctx_, datum.as_num());
                break;
                
            case ql::datum_t::type_t::R_STR:
                duk_push_lstring(ctx_, datum.as_str().data(), 
                                 datum.as_str().size());
                break;
                
            case ql::datum_t::type_t::R_ARRAY: {
                duk_idx_t arr_idx = duk_push_array(ctx_);
                for (size_t i = 0; i < datum.arr_size(); ++i) {
                    push_datum(datum.get(i));
                    duk_put_prop_index(ctx_, arr_idx, static_cast<duk_uarridx_t>(i));
                }
                break;
            }
                
            case ql::datum_t::type_t::R_OBJECT: {
                duk_idx_t obj_idx = duk_push_object(ctx_);
                datum.for_each([&](const ql::datum_string_t& key, 
                                   const ql::datum_t& val) {
                    push_datum(val);
                    duk_put_prop_string(ctx_, obj_idx, key.to_std().c_str());
                    return true;
                });
                break;
            }
                
            default:
                duk_push_undefined(ctx_);
                break;
        }
    }
    
    // Convert Duktape value to datum
    ql::datum_t duk_to_datum(duk_idx_t idx, 
                              const ql::configured_limits_t& limits) {
        idx = duk_normalize_index(ctx_, idx);
        
        if (duk_is_null_or_undefined(ctx_, idx)) {
            return ql::datum_t::null();
        }
        
        if (duk_is_boolean(ctx_, idx)) {
            return ql::datum_t::boolean(duk_get_boolean(ctx_, idx));
        }
        
        if (duk_is_number(ctx_, idx)) {
            return ql::datum_t(duk_get_number(ctx_, idx));
        }
        
        if (duk_is_string(ctx_, idx)) {
            duk_size_t len;
            const char* str = duk_get_lstring(ctx_, idx, &len);
            return ql::datum_t(datum_string_t(std::string(str, len)));
        }
        
        if (duk_is_array(ctx_, idx)) {
            std::vector<ql::datum_t> elements;
            duk_uarridx_t len = duk_get_length(ctx_, idx);
            elements.reserve(len);
            for (duk_uarridx_t i = 0; i < len; ++i) {
                duk_get_prop_index(ctx_, idx, i);
                elements.push_back(duk_to_datum(-1, limits));
                duk_pop(ctx_);
            }
            return ql::datum_t(std::move(elements), limits);
        }
        
        if (duk_is_object(ctx_, idx)) {
            std::map<std::string, ql::datum_t> map;
            duk_enum(ctx_, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
            while (duk_next(ctx_, -1, 1)) {
                const char* key = duk_get_string(ctx_, -2);
                map[key] = duk_to_datum(-1, limits);
                duk_pop_2(ctx_);
            }
            duk_pop(ctx_);
            return ql::datum_t(std::move(map), limits);
        }
        
        return ql::datum_t::null();
    }
    
    DISABLE_COPYING(duktape_engine_t);
};

}  // anonymous namespace

// Factory function
std::unique_ptr<js_engine_t> create_duktape_engine() {
    return std::make_unique<duktape_engine_t>();
}

}  // namespace js
}  // namespace rethinkdb

#endif  // RETHINKDB_JS_ENGINE_DUKTAPE
