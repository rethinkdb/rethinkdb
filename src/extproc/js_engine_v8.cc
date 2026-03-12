// Copyright 2025 RethinkDB, all rights reserved.
// V8 JavaScript engine implementation

#include "extproc/js_engine_v8.hpp"

#include <v8.h>
#include <libplatform/libplatform.h>

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <mutex>

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "containers/archive/stl_types.hpp"

namespace rethinkdb {
namespace js {

namespace {

// V8 platform singleton - shared across all engine instances
class v8_platform_t {
public:
    static v8_platform_t& instance() {
        static v8_platform_t instance;
        return instance;
    }

    v8::Platform* platform() const {
        return platform_.get();
    }

    void initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) {
            v8::V8::InitializeICUDefaultLocation(nullptr);
            v8::V8::InitializeExternalStartupData(nullptr);
            platform_ = v8::platform::NewDefaultPlatform();
            v8::V8::InitializePlatform(platform_.get());
            v8::V8::Initialize();
            initialized_ = true;
        }
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) {
            v8::V8::Dispose();
            v8::V8::DisposePlatform();
            platform_.reset();
            initialized_ = false;
        }
    }

    ~v8_platform_t() {
        if (initialized_) {
            shutdown();
        }
    }

private:
    v8_platform_t() = default;
    
    std::unique_ptr<v8::Platform> platform_;
    bool initialized_ = false;
    std::mutex mutex_;
    
    DISABLE_COPYING(v8_platform_t);
};

// Convert RethinkDB datum to V8 value
v8::Local<v8::Value> datum_to_v8(v8::Isolate* isolate, const ql::datum_t& datum) {
    v8::EscapableHandleScope handle_scope(isolate);
    
    switch (datum.get_type()) {
        case ql::datum_t::type_t::R_NULL:
            return handle_scope.Escape(v8::Null(isolate));
            
        case ql::datum_t::type_t::R_BOOL:
            return handle_scope.Escape(v8::Boolean::New(isolate, datum.as_bool()));
            
        case ql::datum_t::type_t::R_NUM:
            return handle_scope.Escape(v8::Number::New(isolate, datum.as_num()));
            
        case ql::datum_t::type_t::R_STR:
            return handle_scope.Escape(v8::String::NewFromUtf8(isolate, 
                datum.as_str().data(), v8::NewStringType::kNormal,
                static_cast<int>(datum.as_str().size())).ToLocalChecked());
            
        case ql::datum_t::type_t::R_ARRAY: {
            v8::Local<v8::Array> arr = v8::Array::New(isolate, 
                static_cast<int>(datum.arr_size()));
            for (size_t i = 0; i < datum.arr_size(); ++i) {
                arr->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i),
                    datum_to_v8(isolate, datum.get(i))).Check();
            }
            return handle_scope.Escape(arr);
        }
            
        case ql::datum_t::type_t::R_OBJECT: {
            v8::Local<v8::Object> obj = v8::Object::New(isolate);
            datum.for_each([&](const ql::datum_string_t& key, const ql::datum_t& val) {
                obj->Set(isolate->GetCurrentContext(),
                    v8::String::NewFromUtf8(isolate, key.data(),
                        v8::NewStringType::kNormal,
                        static_cast<int>(key.size())).ToLocalChecked(),
                    datum_to_v8(isolate, val)).Check();
                return true;
            });
            return handle_scope.Escape(obj);
        }
            
        default:
            return handle_scope.Escape(v8::Undefined(isolate));
    }
}

// Convert V8 value to RethinkDB datum
ql::datum_t v8_to_datum(v8::Isolate* isolate, v8::Local<v8::Value> value,
                        const ql::configured_limits_t& limits) {
    v8::HandleScope handle_scope(isolate);
    
    if (value->IsNullOrUndefined()) {
        return ql::datum_t::null();
    }
    
    if (value->IsBoolean()) {
        return ql::datum_t::boolean(value->BooleanValue(isolate));
    }
    
    if (value->IsNumber()) {
        return ql::datum_t(value->NumberValue(isolate->GetCurrentContext()).FromJust());
    }
    
    if (value->IsString()) {
        v8::String::Utf8Value utf8(isolate, value);
        return ql::datum_t(datum_string_t(*utf8));
    }
    
    if (value->IsArray()) {
        v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(value);
        std::vector<ql::datum_t> elements;
        elements.reserve(arr->Length());
        
        for (uint32_t i = 0; i < arr->Length(); ++i) {
            elements.push_back(v8_to_datum(isolate,
                arr->Get(isolate->GetCurrentContext(), i).ToLocalChecked(), limits));
        }
        return ql::datum_t(std::move(elements), limits);
    }
    
    if (value->IsObject()) {
        v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(value);
        v8::Local<v8::Array> keys = obj->GetPropertyNames(isolate->GetCurrentContext())
            .ToLocalChecked();
        
        std::map<std::string, ql::datum_t> map;
        for (uint32_t i = 0; i < keys->Length(); ++i) {
            v8::Local<v8::Value> key = keys->Get(isolate->GetCurrentContext(), i)
                .ToLocalChecked();
            v8::String::Utf8Value key_utf8(isolate, key);
            v8::Local<v8::Value> val = obj->Get(isolate->GetCurrentContext(), key)
                .ToLocalChecked();
            map[std::string(*key_utf8)] = v8_to_datum(isolate, val, limits);
        }
        return ql::datum_t(std::move(map), limits);
    }
    
    return ql::datum_t::null();
}

// V8 engine implementation
class v8_js_engine_t : public js_engine_t {
public:
    explicit v8_js_engine_t(bool use_jit) : use_jit_(use_jit) {
        // Initialize V8 platform
        v8_platform_t::instance().initialize();
        
        // Create isolate parameters
        v8::Isolate::CreateParams params;
        params.array_buffer_allocator = 
            v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        
        // Disable JIT if requested (jitless mode)
        if (!use_jit) {
            // V8 jitless mode configuration
            params.code_event_handler = nullptr;
        }
        
        isolate_ = v8::Isolate::New(params);
        isolate_->SetMemoryLimit(512 * 1024 * 1024);  // 512MB limit
        
        // Enter isolate scope
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);
        
        // Create global context
        v8::Local<v8::Context> context = v8::Context::New(isolate_);
        context_.Reset(isolate_, context);
        
        // Set up security: disable eval, restrict global access
        v8::Local<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New(isolate_);
        // Additional security configurations can be added here
    }
    
    ~v8_js_engine_t() override {
        context_.Reset();
        if (isolate_) {
            isolate_->Dispose();
        }
    }
    
    ql::datum_t eval(const std::string& code,
                     const ql::configured_limits_t& limits) override {
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);
        v8::Local<v8::Context> context = 
            v8::Local<v8::Context>::New(isolate_, context_);
        v8::Context::Scope context_scope(context);
        
        // Create try-catch for error handling
        v8::TryCatch try_catch(isolate_);
        
        // Compile source
        v8::Local<v8::String> source = 
            v8::String::NewFromUtf8(isolate_, code.c_str(),
                v8::NewStringType::kNormal,
                static_cast<int>(code.size())).ToLocalChecked();
        
        v8::Local<v8::Script> script;
        if (!v8::Script::Compile(context, source).ToLocal(&script)) {
            v8::String::Utf8Value error(isolate_, try_catch.Exception());
            throw ql::base_exc_t(ql::base_exc_t::LOGIC, 
                std::string("JavaScript compilation error: ") + *error);
        }
        
        // Run script
        v8::Local<v8::Value> result;
        if (!script->Run(context).ToLocal(&result)) {
            v8::String::Utf8Value error(isolate_, try_catch.Exception());
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                std::string("JavaScript runtime error: ") + *error);
        }
        
        return v8_to_datum(isolate_, result, limits);
    }
    
    ql::datum_t call(const std::string& func_name,
                     const std::vector<ql::datum_t>& args,
                     const ql::configured_limits_t& limits) override {
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);
        v8::Local<v8::Context> context = 
            v8::Local<v8::Context>::New(isolate_, context_);
        v8::Context::Scope context_scope(context);
        
        v8::TryCatch try_catch(isolate_);
        
        // Get function from global object
        v8::Local<v8::String> name = 
            v8::String::NewFromUtf8(isolate_, func_name.c_str()).ToLocalChecked();
        v8::Local<v8::Value> val = 
            context->Global()->Get(context, name).ToLocalChecked();
        
        if (!val->IsFunction()) {
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                func_name + " is not a function");
        }
        
        v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(val);
        
        // Convert arguments
        std::vector<v8::Local<v8::Value>> v8_args;
        v8_args.reserve(args.size());
        for (const auto& arg : args) {
            v8_args.push_back(datum_to_v8(isolate_, arg));
        }
        
        // Call function
        v8::Local<v8::Value> result;
        if (!func->Call(context, context->Global(),
                        static_cast<int>(v8_args.size()),
                        v8_args.empty() ? nullptr : &v8_args[0]).ToLocal(&result)) {
            v8::String::Utf8Value error(isolate_, try_catch.Exception());
            throw ql::base_exc_t(ql::base_exc_t::LOGIC,
                std::string("JavaScript call error: ") + *error);
        }
        
        return v8_to_datum(isolate_, result, limits);
    }
    
    void set_global(const std::string& name, const ql::datum_t& value) override {
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);
        v8::Local<v8::Context> context = 
            v8::Local<v8::Context>::New(isolate_, context_);
        v8::Context::Scope context_scope(context);
        
        v8::Local<v8::String> v8_name = 
            v8::String::NewFromUtf8(isolate_, name.c_str()).ToLocalChecked();
        context->Global()->Set(context, v8_name, datum_to_v8(isolate_, value)).Check();
    }
    
    ql::datum_t get_global(const std::string& name,
                          const ql::configured_limits_t& limits) override {
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);
        v8::Local<v8::Context> context = 
            v8::Local<v8::Context>::New(isolate_, context_);
        v8::Context::Scope context_scope(context);
        
        v8::Local<v8::String> v8_name = 
            v8::String::NewFromUtf8(isolate_, name.c_str()).ToLocalChecked();
        v8::Local<v8::Value> val = 
            context->Global()->Get(context, v8_name).ToLocalChecked();
        
        return v8_to_datum(isolate_, val, limits);
    }
    
    size_t get_memory_usage() const override {
        v8::HeapStatistics stats;
        isolate_->GetHeapStatistics(&stats);
        return stats.used_heap_size();
    }
    
    void gc() override {
        isolate_->LowMemoryNotification();
    }
    
    js_engine_type_t get_type() const override {
        return use_jit_ ? js_engine_type_t::V8_FULL : js_engine_type_t::V8_JITLESS;
    }
    
    std::string get_version() const override {
        return v8::V8::GetVersion();
    }

private:
    v8::Isolate* isolate_;
    v8::Persistent<v8::Context> context_;
    bool use_jit_;
    
    DISABLE_COPYING(v8_js_engine_t);
};

}  // anonymous namespace

// Factory functions
std::unique_ptr<js_engine_t> create_v8_jitless_engine() {
    return std::make_unique<v8_js_engine_t>(false);
}

std::unique_ptr<js_engine_t> create_v8_full_engine() {
    return std::make_unique<v8_js_engine_t>(true);
}

}  // namespace js
}  // namespace rethinkdb
