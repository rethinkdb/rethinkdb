// Copyright 2025 RethinkDB, all rights reserved.
// Hermes JavaScript engine implementation

#include "extproc/js_engine.hpp"

#if defined(RETHINKDB_JS_ENGINE_HERMES)

#include <hermes/hermes.h>
#include <jsi/jsi.h>

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

using namespace facebook::jsi;

// Hermes engine implementation
class hermes_engine_t : public js_engine_t {
public:
    hermes_engine_t() {
        // Create Hermes runtime with security settings
        facebook::hermes::vm::RuntimeConfig config;
        config.withMaxHeapSize(512 << 20);  // 512MB heap limit
        
        runtime_ = facebook::hermes::makeHermesRuntime(config);
        if (!runtime_) {
            throw std::runtime_error("Failed to create Hermes runtime");
        }
    }
    
    ~hermes_engine_t() override = default;
    
    ql::datum_t eval(const std::string& code,
                     const ql::configured_limits_t& limits) override {
        try {
            Value result = runtime_->evaluateJavaScript(
                std::make_shared<StringBuffer>(code), "<input>");
            return jsi_to_datum(result, limits);
        } catch (const JSError& e) {
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                std::string("JavaScript error: ") + e.what());
        }
    }
    
    ql::datum_t call(const std::string& func_name,
                     const std::vector<ql::datum_t>& args,
                     const ql::configured_limits_t& limits) override {
        try {
            Object global = runtime_->global();
            Value func = global.getProperty(*runtime_, func_name.c_str());
            
            if (!func.isObject() || !func.asObject(*runtime_).isFunction(*runtime_)) {
                throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                    func_name + " is not a function");
            }
            
            // Convert arguments
            std::vector<Value> jsi_args;
            jsi_args.reserve(args.size());
            for (const auto& arg : args) {
                jsi_args.push_back(datum_to_jsi(arg));
            }
            
            Value result = func.asObject(*runtime_).callAsFunction(
                *runtime_, global, jsi_args.data(), jsi_args.size());
            
            return jsi_to_datum(result, limits);
        } catch (const JSError& e) {
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                std::string("JavaScript call error: ") + e.what());
        }
    }
    
    void set_global(const std::string& name, const ql::datum_t& value) override {
        Object global = runtime_->global();
        global.setProperty(*runtime_, name.c_str(), datum_to_jsi(value));
    }
    
    ql::datum_t get_global(const std::string& name,
                          const ql::configured_limits_t& limits) override {
        Object global = runtime_->global();
        Value val = global.getProperty(*runtime_, name.c_str());
        return jsi_to_datum(val, limits);
    }
    
    size_t get_memory_usage() const override {
        // Hermes doesn't expose detailed memory stats directly
        // Return heap size estimate
        return 0;  // Placeholder
    }
    
    void gc() override {
        // Hermes has automatic GC, but we can hint at it
        // Note: Hermes GC is automatic, no explicit call available
    }
    
    js_engine_type_t get_type() const override {
        return js_engine_type_t::HERMES;
    }
    
    std::string get_version() const override {
        return "Hermes";
    }

private:
    std::unique_ptr<Runtime> runtime_;
    
    // Convert datum to JSI Value
    Value datum_to_jsi(const ql::datum_t& datum) {
        switch (datum.get_type()) {
            case ql::datum_t::type_t::R_NULL:
                return Value::null();
                
            case ql::datum_t::type_t::R_BOOL:
                return Value(*runtime_, datum.as_bool());
                
            case ql::datum_t::type_t::R_NUM:
                return Value(*runtime_, datum.as_num());
                
            case ql::datum_t::type_t::R_STR:
                return Value(*runtime_, String::createFromUtf8(
                    *runtime_, datum.as_str().data(), datum.as_str().size()));
                
            case ql::datum_t::type_t::R_ARRAY: {
                Object arr = Array::createWithElements(*runtime_, {});
                for (size_t i = 0; i < datum.arr_size(); ++i) {
                    arr.setPropertyAtIndex(*runtime_, static_cast<size_t>(i),
                                           datum_to_jsi(datum.get(i)));
                }
                return Value(*runtime_, arr);
            }
                
            case ql::datum_t::type_t::R_OBJECT: {
                Object obj(*runtime_);
                datum.for_each([&](const ql::datum_string_t& key,
                                   const ql::datum_t& val) {
                    obj.setProperty(*runtime_, PropNameID::forAscii(*runtime_,
                        key.data(), static_cast<size_t>(key.size())),
                        datum_to_jsi(val));
                    return true;
                });
                return Value(*runtime_, obj);
            }
                
            default:
                return Value::undefined();
        }
    }
    
    // Convert JSI Value to datum
    ql::datum_t jsi_to_datum(const Value& value,
                              const ql::configured_limits_t& limits) {
        if (value.isNull() || value.isUndefined()) {
            return ql::datum_t::null();
        }
        
        if (value.isBool()) {
            return ql::datum_t::boolean(value.getBool());
        }
        
        if (value.isNumber()) {
            return ql::datum_t(value.getNumber());
        }
        
        if (value.isString()) {
            std::string str = value.getString(*runtime_).utf8(*runtime_);
            return ql::datum_t(datum_string_t(str));
        }
        
        if (value.isObject()) {
            Object obj = value.getObject(*runtime_);
            
            if (obj.isArray(*runtime_)) {
                Array arr = obj.getArray(*runtime_);
                size_t len = arr.size(*runtime_);
                std::vector<ql::datum_t> elements;
                elements.reserve(len);
                for (size_t i = 0; i < len; ++i) {
                    elements.push_back(jsi_to_datum(
                        arr.getValueAtIndex(*runtime_, i), limits));
                }
                return ql::datum_t(std::move(elements), limits);
            }
            
            std::map<std::string, ql::datum_t> map;
            Array names = obj.getPropertyNames(*runtime_);
            for (size_t i = 0; i < names.size(*runtime_); ++i) {
                std::string key = names.getValueAtIndex(*runtime_, i)
                    .getString(*runtime_).utf8(*runtime_);
                map[key] = jsi_to_datum(obj.getProperty(*runtime_, key.c_str()),
                                        limits);
            }
            return ql::datum_t(std::move(map), limits);
        }
        
        return ql::datum_t::null();
    }
    
    DISABLE_COPYING(hermes_engine_t);
};

}  // anonymous namespace

// Factory function
std::unique_ptr<js_engine_t> create_hermes_engine() {
    return std::make_unique<hermes_engine_t>();
}

}  // namespace js
}  // namespace rethinkdb

#endif  // RETHINKDB_JS_ENGINE_HERMES
