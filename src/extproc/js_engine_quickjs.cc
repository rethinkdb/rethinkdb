// Copyright 2025 RethinkDB, all rights reserved.
// QuickJS JavaScript engine implementation

#include "extproc/js_engine.hpp"

#if defined(RETHINKDB_JS_ENGINE_QUICKJS) || defined(RETHINKDB_JS_ENGINE_QUICKJS_NG)

#include <quickjs.h>

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

// QuickJS engine implementation
class quickjs_engine_t : public js_engine_t {
public:
    explicit quickjs_engine_t(bool use_ng) : use_ng_(use_ng) {
        runtime_ = JS_NewRuntime();
        if (!runtime_) {
            throw std::runtime_error("Failed to create QuickJS runtime");
        }
        
        // Set memory limit
        JS_SetMemoryLimit(runtime_, 512 * 1024 * 1024);  // 512MB
        
        // Set max stack size
        JS_SetMaxStackSize(runtime_, 1024 * 1024);  // 1MB
        
        context_ = JS_NewContext(runtime_);
        if (!context_) {
            JS_FreeRuntime(runtime_);
            throw std::runtime_error("Failed to create QuickJS context");
        }
    }
    
    ~quickjs_engine_t() override {
        if (context_) {
            JS_FreeContext(context_);
        }
        if (runtime_) {
            JS_FreeRuntime(runtime_);
        }
    }
    
    ql::datum_t eval(const std::string& code,
                     const ql::configured_limits_t& limits) override {
        JSValue result = JS_Eval(context_, code.c_str(), code.size(),
                                 "<input>", JS_EVAL_TYPE_GLOBAL);
        
        if (JS_IsException(result)) {
            JSValue exception = JS_GetException(context_);
            const char* str = JS_ToCString(context_, exception);
            std::string error = str ? str : "Unknown error";
            JS_FreeCString(context_, str);
            JS_FreeValue(context_, exception);
            JS_FreeValue(context_, result);
            throw ql::base_exc_t(ql::base_exc_t::LOGIC, 
                std::string("JavaScript error: ") + error);
        }
        
        ql::datum_t datum = jsvalue_to_datum(result, limits);
        JS_FreeValue(context_, result);
        return datum;
    }
    
    ql::datum_t call(const std::string& func_name,
                     const std::vector<ql::datum_t>& args,
                     const ql::configured_limits_t& limits) override {
        JSValue global = JS_GetGlobalObject(context_);
        JSValue func = JS_GetPropertyStr(context_, global, func_name.c_str());
        
        if (!JS_IsFunction(context_, func)) {
            JS_FreeValue(context_, func);
            JS_FreeValue(context_, global);
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                func_name + " is not a function");
        }
        
        // Convert arguments
        std::vector<JSValue> js_args;
        js_args.reserve(args.size());
        for (const auto& arg : args) {
            js_args.push_back(datum_to_jsvalue(arg));
        }
        
        JSValue result = JS_Call(context_, func, global,
                                 static_cast<int>(js_args.size()),
                                 js_args.empty() ? nullptr : js_args.data());
        
        // Free arguments
        for (auto& arg : js_args) {
            JS_FreeValue(context_, arg);
        }
        JS_FreeValue(context_, func);
        JS_FreeValue(context_, global);
        
        if (JS_IsException(result)) {
            JSValue exception = JS_GetException(context_);
            const char* str = JS_ToCString(context_, exception);
            std::string error = str ? str : "Unknown error";
            JS_FreeCString(context_, str);
            JS_FreeValue(context_, exception);
            JS_FreeValue(context_, result);
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                std::string("JavaScript call error: ") + error);
        }
        
        ql::datum_t datum = jsvalue_to_datum(result, limits);
        JS_FreeValue(context_, result);
        return datum;
    }
    
    void set_global(const std::string& name, const ql::datum_t& value) override {
        JSValue global = JS_GetGlobalObject(context_);
        JSValue val = datum_to_jsvalue(value);
        JS_SetPropertyStr(context_, global, name.c_str(), val);
        JS_FreeValue(context_, global);
    }
    
    ql::datum_t get_global(const std::string& name,
                          const ql::configured_limits_t& limits) override {
        JSValue global = JS_GetGlobalObject(context_);
        JSValue val = JS_GetPropertyStr(context_, global, name.c_str());
        JS_FreeValue(context_, global);
        
        ql::datum_t datum = jsvalue_to_datum(val, limits);
        JS_FreeValue(context_, val);
        return datum;
    }
    
    size_t get_memory_usage() const override {
        JSMemoryUsage stats;
        JS_ComputeMemoryUsage(runtime_, &stats);
        return stats.memory_used_size;
    }
    
    void gc() override {
        JS_RunGC(runtime_);
    }
    
    js_engine_type_t get_type() const override {
        return use_ng_ ? js_engine_type_t::QUICKJS_NG : js_engine_type_t::QUICKJS;
    }
    
    std::string get_version() const override {
        return use_ng_ ? "QuickJS-NG" : "QuickJS";
    }

private:
    JSRuntime* runtime_ = nullptr;
    JSContext* context_ = nullptr;
    bool use_ng_;
    
    // Convert datum to JSValue
    JSValue datum_to_jsvalue(const ql::datum_t& datum) {
        switch (datum.get_type()) {
            case ql::datum_t::type_t::R_NULL:
                return JS_NULL;
                
            case ql::datum_t::type_t::R_BOOL:
                return JS_NewBool(context_, datum.as_bool());
                
            case ql::datum_t::type_t::R_NUM:
                return JS_NewFloat64(context_, datum.as_num());
                
            case ql::datum_t::type_t::R_STR:
                return JS_NewStringLen(context_, datum.as_str().data(),
                                       datum.as_str().size());
                
            case ql::datum_t::type_t::R_ARRAY: {
                JSValue arr = JS_NewArray(context_);
                for (size_t i = 0; i < datum.arr_size(); ++i) {
                    JSValue val = datum_to_jsvalue(datum.get(i));
                    JS_SetPropertyUint32(context_, arr, static_cast<uint32_t>(i), val);
                }
                return arr;
            }
                
            case ql::datum_t::type_t::R_OBJECT: {
                JSValue obj = JS_NewObject(context_);
                datum.for_each([&](const ql::datum_string_t& key, 
                                   const ql::datum_t& val) {
                    JSValue v = datum_to_jsvalue(val);
                    JS_SetPropertyStr(context_, obj, key.to_std().c_str(), v);
                    return true;
                });
                return obj;
            }
                
            default:
                return JS_UNDEFINED;
        }
    }
    
    // Convert JSValue to datum
    ql::datum_t jsvalue_to_datum(JSValue val, 
                                  const ql::configured_limits_t& limits) {
        if (JS_IsNull(val) || JS_IsUndefined(val)) {
            return ql::datum_t::null();
        }
        
        if (JS_IsBool(val)) {
            return ql::datum_t::boolean(JS_ToBool(context_, val));
        }
        
        if (JS_IsNumber(val)) {
            double num;
            JS_ToFloat64(context_, &num, val);
            return ql::datum_t(num);
        }
        
        if (JS_IsString(val)) {
            const char* str = JS_ToCString(context_, val);
            ql::datum_t result = ql::datum_t(datum_string_t(str));
            JS_FreeCString(context_, str);
            return result;
        }
        
        if (JS_IsArray(context_, val)) {
            int64_t len;
            JS_GetPropertyLength(context_, &len, val);
            std::vector<ql::datum_t> elements;
            elements.reserve(len);
            for (int64_t i = 0; i < len; ++i) {
                JSValue elem = JS_GetPropertyUint32(context_, val, static_cast<uint32_t>(i));
                elements.push_back(jsvalue_to_datum(elem, limits));
                JS_FreeValue(context_, elem);
            }
            return ql::datum_t(std::move(elements), limits);
        }
        
        if (JS_IsObject(val)) {
            JSPropertyEnum* tabs = nullptr;
            uint32_t len = 0;
            JS_GetOwnPropertyNames(context_, &tabs, &len, val, 
                                   JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY);
            
            std::map<std::string, ql::datum_t> map;
            for (uint32_t i = 0; i < len; ++i) {
                const char* key = JS_AtomToCString(context_, tabs[i].atom);
                JSValue prop = JS_GetProperty(context_, val, tabs[i].atom);
                map[key] = jsvalue_to_datum(prop, limits);
                JS_FreeCString(context_, key);
                JS_FreeValue(context_, prop);
                JS_FreeAtom(context_, tabs[i].atom);
            }
            js_free(context_, tabs);
            return ql::datum_t(std::move(map), limits);
        }
        
        return ql::datum_t::null();
    }
    
    DISABLE_COPYING(quickjs_engine_t);
};

}  // anonymous namespace

// Factory functions
std::unique_ptr<js_engine_t> create_quickjs_engine() {
    return std::make_unique<quickjs_engine_t>(false);
}

std::unique_ptr<js_engine_t> create_quickjs_ng_engine() {
    return std::make_unique<quickjs_engine_t>(true);
}

}  // namespace js
}  // namespace rethinkdb

#endif  // RETHINKDB_JS_ENGINE_QUICKJS || RETHINKDB_JS_ENGINE_QUICKJS_NG
