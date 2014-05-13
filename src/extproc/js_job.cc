// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "extproc/js_job.hpp"

#include <stdint.h>

#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 406)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include <v8.h>
#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 406)
#pragma GCC diagnostic pop
#endif

#include <cmath>
#include <limits>

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "extproc/extproc_job.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "rdb_protocol/pseudo_time.hpp"

#ifdef V8_PRE_3_19
#define DECLARE_HANDLE_SCOPE(scope) v8::HandleScope scope
#else
#define DECLARE_HANDLE_SCOPE(scope) v8::HandleScope scope(v8::Isolate::GetCurrent())
#endif


const js_id_t MIN_ID = 1;
const js_id_t MAX_ID = std::numeric_limits<js_id_t>::max();

// Picked from a hat.
#define TO_JSON_RECURSION_LIMIT  500

// Returns an empty counted_t on error.
counted_t<const ql::datum_t> js_to_datum(const v8::Handle<v8::Value> &value,
                                         std::string *errmsg);

// Should never error.
v8::Handle<v8::Value> js_from_datum(const counted_t<const ql::datum_t> &datum);

// Worker-side JS evaluation environment.
class js_env_t {
public:
    js_env_t();
    ~js_env_t();

    js_result_t eval(const std::string &source);
    js_result_t call(js_id_t id, const std::vector<counted_t<const ql::datum_t> > &args);
    void release(js_id_t id);

private:
    js_id_t remember_value(const v8::Handle<v8::Value> &value);
    const boost::shared_ptr<v8::Persistent<v8::Value> > find_value(js_id_t id);

    js_id_t next_id;
    std::map<js_id_t, boost::shared_ptr<v8::Persistent<v8::Value> > > values;
};

// Cleans the worker process's environment when instantiated
class js_context_t {
public:
#ifdef V8_PRE_3_19
    js_context_t() :
        context(v8::Context::New()),
        scope(context) { }

    ~js_context_t() {
        context.Dispose();
    }

    v8::Persistent<v8::Context> context;
#else
    js_context_t() :
        local_scope(v8::Isolate::GetCurrent()),
        context(v8::Context::New(v8::Isolate::GetCurrent())),
        scope(context) { }

    v8::HandleScope local_scope;
    v8::Local<v8::Context> context;
#endif
    v8::Context::Scope scope;
};

enum js_task_t {
    TASK_EVAL,
    TASK_CALL,
    TASK_RELEASE,
    TASK_EXIT
};

// The job_t runs in the context of the main rethinkdb process
js_job_t::js_job_t(extproc_pool_t *pool, signal_t *interruptor) :
    extproc_job(pool, &worker_fn, interruptor) { }

js_result_t js_job_t::eval(const std::string &source) {
    js_task_t task = js_task_t::TASK_EVAL;
    write_message_t wm;
    wm.append(&task, sizeof(task));
    serialize(&wm, source);
    {
        int res = send_write_message(extproc_job.write_stream(), &wm);
        if (res != 0) { throw js_worker_exc_t("failed to send data to the worker"); }
    }

    js_result_t result;
    archive_result_t res = deserialize(extproc_job.read_stream(), &result);
    if (bad(res)) {
        throw js_worker_exc_t(strprintf("failed to deserialize result from worker (%s)",
                                        archive_result_as_str(res)));
    }
    return result;
}

js_result_t js_job_t::call(js_id_t id, const std::vector<counted_t<const ql::datum_t> > &args) {
    js_task_t task = js_task_t::TASK_CALL;
    write_message_t wm;
    wm.append(&task, sizeof(task));
    serialize(&wm, id);
    serialize(&wm, args);
    {
        int res = send_write_message(extproc_job.write_stream(), &wm);
        if (res != 0) { throw js_worker_exc_t("failed to send data to the worker"); }
    }

    js_result_t result;
    archive_result_t res = deserialize(extproc_job.read_stream(), &result);
    if (bad(res)) {
        throw js_worker_exc_t(strprintf("failed to deserialize result from worker (%s)",
                                        archive_result_as_str(res)));
    }
    return result;
}

void js_job_t::release(js_id_t id) {
    js_task_t task = js_task_t::TASK_RELEASE;
    write_message_t wm;
    wm.append(&task, sizeof(task));
    serialize(&wm, id);
    int res = send_write_message(extproc_job.write_stream(), &wm);
    if (res != 0) { throw js_worker_exc_t("failed to send data to the worker"); }
}

void js_job_t::exit() {
    js_task_t task = js_task_t::TASK_EXIT;
    write_message_t wm;
    wm.append(&task, sizeof(task));
    int res = send_write_message(extproc_job.write_stream(), &wm);
    if (res != 0) { throw js_worker_exc_t("failed to send data to the worker"); }
}

void js_job_t::worker_error() {
    extproc_job.worker_error();
}

bool js_job_t::worker_fn(read_stream_t *stream_in, write_stream_t *stream_out) {
    bool running = true;
    js_env_t js_env;

    while (running) {
        js_task_t task;
        int64_t read_size = sizeof(task);
        {
            int64_t res = force_read(stream_in, &task, read_size);
            if (res != read_size) { return false; }
        }

        switch (task) {
        case TASK_EVAL:
            {
                std::string source;
                {
                    archive_result_t res = deserialize(stream_in, &source);
                    if (bad(res)) { return false; }
                }

                js_result_t js_result = js_env.eval(source);
                write_message_t wm;
                serialize(&wm, js_result);
                int res = send_write_message(stream_out, &wm);
                if (res != 0) { return false; }
            }
            break;
        case TASK_CALL:
            {
                js_id_t id;
                std::vector<counted_t<const ql::datum_t> > args;
                {
                    archive_result_t res = deserialize(stream_in, &id);
                    if (bad(res)) { return false; }
                    res = deserialize(stream_in, &args);
                    if (bad(res)) { return false; }
                }

                js_result_t js_result = js_env.call(id, args);
                write_message_t wm;
                serialize(&wm, js_result);
                int res = send_write_message(stream_out, &wm);
                if (res != 0) { return false; }
            }
            break;
        case TASK_RELEASE:
            {
                js_id_t id;
                archive_result_t res = deserialize(stream_in, &id);
                if (bad(res)) { return false; }
                js_env.release(id);
            }
            break;
        case TASK_EXIT:
            return true;
            break;
        default:
            return false;
        }
    }
    unreachable();
}

static void append_caught_error(std::string *errmsg, const v8::TryCatch &try_catch) {
    if (!try_catch.HasCaught()) return;

    v8::String::Utf8Value exception(try_catch.Exception());
    const char *message = *exception;
    guarantee(message);
    errmsg->append(message, strlen(message));
}

// The env_t runs in the context of the worker process
js_env_t::js_env_t() :
    next_id(MIN_ID) { }

js_env_t::~js_env_t() {
    // Clean up handles.
    for (auto it = values.begin(); it != values.end(); ++it) {
        it->second->Dispose();
    }
}

js_result_t js_env_t::eval(const std::string &source) {
    js_context_t clean_context;
    js_result_t result("");
    std::string *errmsg = boost::get<std::string>(&result);

    DECLARE_HANDLE_SCOPE(handle_scope);

    // TODO: use an "external resource" to avoid copy?
    v8::Handle<v8::String> src = v8::String::New(source.data(), source.size());

    // This constructor registers itself with v8 so that any errors generated
    // within v8 will be available within this object.
    v8::TryCatch try_catch;

    // Firstly, compilation may fail (because of say a syntax error)
    v8::Handle<v8::Script> script = v8::Script::Compile(src);
    if (script.IsEmpty()) {
        // Get the error out of the TryCatch object
        append_caught_error(errmsg, try_catch);
    } else {
        // Secondly, evaluation may fail because of an exception generated
        // by the code
        v8::Handle<v8::Value> result_val = script->Run();
        if (result_val.IsEmpty()) {
            // Get the error from the TryCatch object
            append_caught_error(errmsg, try_catch);
        } else {
            // Scripts that evaluate to functions become RQL Func terms that
            // can be passed to map, filter, reduce, etc.
            if (result_val->IsFunction()) {
                v8::Handle<v8::Function> func
                    = v8::Handle<v8::Function>::Cast(result_val);
                result = remember_value(func);
            } else {
                guarantee(!result_val.IsEmpty());

                // JSONify result.
                counted_t<const ql::datum_t> datum = js_to_datum(result_val, errmsg);
                if (datum.has()) {
                    result = datum;
                }
            }
        }
    }

    return result;
}

js_id_t js_env_t::remember_value(const v8::Handle<v8::Value> &value) {
    guarantee(next_id < MAX_ID);
    js_id_t id = next_id++;

    // Save this value in a persistent handle so it isn't deallocated when
    // its scope is destructed.

    boost::shared_ptr<v8::Persistent<v8::Value> > persistent_handle(new v8::Persistent<v8::Value>());
#ifdef V8_PRE_3_19
    *persistent_handle = v8::Persistent<v8::Value>::New(value);
#else
    persistent_handle->Reset(v8::Isolate::GetCurrent(), value);
#endif

    values.insert(std::make_pair(id, persistent_handle));
    return id;
}

const boost::shared_ptr<v8::Persistent<v8::Value> > js_env_t::find_value(js_id_t id) {
    std::map<js_id_t, boost::shared_ptr<v8::Persistent<v8::Value> > >::iterator it = values.find(id);
    guarantee(it != values.end());
    return it->second;
}

v8::Handle<v8::Value> run_js_func(v8::Handle<v8::Function> fn,
                                  const std::vector<counted_t<const ql::datum_t> > &args,
                                  std::string *errmsg) {
    v8::TryCatch try_catch;
    DECLARE_HANDLE_SCOPE(scope);

    // Construct receiver object.
    v8::Handle<v8::Object> obj = v8::Object::New();
    guarantee(!obj.IsEmpty());

    // Construct arguments.
    scoped_array_t<v8::Handle<v8::Value> > handles(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        handles[i] = js_from_datum(args[i]);
        guarantee(!handles[i].IsEmpty());
    }

    // Call function with environment as its receiver.
    v8::Handle<v8::Value> result = fn->Call(obj, args.size(), handles.data());
    if (result.IsEmpty()) {
        append_caught_error(errmsg, try_catch);
    }
    return scope.Close(result);
}

js_result_t js_env_t::call(js_id_t id,
                           const std::vector<counted_t<const ql::datum_t> > &args) {
    js_context_t clean_context;
    js_result_t result("");
    std::string *errmsg = boost::get<std::string>(&result);

    const boost::shared_ptr<v8::Persistent<v8::Value> > found_value = find_value(id);
    guarantee(!found_value->IsEmpty());

    DECLARE_HANDLE_SCOPE(handle_scope);

    // Construct local handle from persistent handle


#ifdef V8_PRE_3_19
    v8::Local<v8::Value> local_handle = v8::Local<v8::Value>::New(*found_value);
#else
    v8::Local<v8::Value> local_handle = v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), *found_value);
#endif
    v8::Local<v8::Function> fn = v8::Local<v8::Function>::Cast(local_handle);
    v8::Handle<v8::Value> value = run_js_func(fn, args, errmsg);

    if (!value.IsEmpty()) {
        if (value->IsFunction()) {
            v8::Handle<v8::Function> sub_func = v8::Handle<v8::Function>::Cast(value);
            result = remember_value(sub_func);
        } else {
            // JSONify result.
            counted_t<const ql::datum_t> datum = js_to_datum(value, errmsg);
            if (datum.has()) {
                result = datum;
            }
        }
    }
    return result;
}

void js_env_t::release(js_id_t id) {
    guarantee(id < next_id);
    size_t num_erased = values.erase(id);
    guarantee(1 == num_erased);
}

// TODO: Is there a better way of detecting circular references than a recursion limit?
counted_t<const ql::datum_t> js_make_datum(const v8::Handle<v8::Value> &value,
                                           int recursion_limit,
                                           std::string *errmsg) {
    counted_t<const ql::datum_t> result;

    if (0 == recursion_limit) {
        errmsg->assign("Recursion limit exceeded in js_to_json (circular reference?).");
        return result;
    }
    --recursion_limit;

    // TODO: should we handle BooleanObject, NumberObject, StringObject?
    DECLARE_HANDLE_SCOPE(handle_scope);

    if (value->IsString()) {
        v8::Handle<v8::String> string = value->ToString();
        guarantee(!string.IsEmpty());

        size_t length = string->Utf8Length();
        scoped_array_t<char> temp_buffer(length);
        string->WriteUtf8(temp_buffer.data(), length);

        try {
            result = make_counted<const ql::datum_t>(std::string(temp_buffer.data(), length));
        } catch (const ql::base_exc_t &ex) {
            errmsg->assign(ex.what());
        }
    } else if (value->IsObject()) {
        // This case is kinda weird. Objects can have stuff in them that isn't
        // represented in their JSON (eg. their prototype, v8 hidden fields).

        if (value->IsArray()) {
            v8::Handle<v8::Array> arrayh = v8::Handle<v8::Array>::Cast(value);
            std::vector<counted_t<const ql::datum_t> > datum_array;
            datum_array.reserve(arrayh->Length());

            for (uint32_t i = 0; i < arrayh->Length(); ++i) {
                v8::Handle<v8::Value> elth = arrayh->Get(i);
                guarantee(!elth.IsEmpty());

                counted_t<const ql::datum_t> item = js_make_datum(elth, recursion_limit, errmsg);
                if (!item.has()) {
                    // Result is still empty, the error message has been set
                    return result;
                }
                datum_array.push_back(std::move(item));
            }

            result = make_counted<const ql::datum_t>(std::move(datum_array));
        } else if (value->IsFunction()) {
            // We can't represent functions in JSON.
            errmsg->assign("Cannot convert function to ql::datum_t.");
        } else if (value->IsRegExp()) {
            // We can't represent regular expressions in datums
            errmsg->assign("Cannot convert RegExp to ql::datum_t.");
        } else if (value->IsDate()) {
            result = ql::pseudo::make_time(value->NumberValue() / 1000,
                                           "+00:00");
        } else {
            // Treat it as a dictionary.
            v8::Handle<v8::Object> objh = value->ToObject();
            guarantee(!objh.IsEmpty());
            v8::Handle<v8::Array> properties = objh->GetPropertyNames();
            guarantee(!properties.IsEmpty());

            std::map<std::string, counted_t<const ql::datum_t> > datum_map;

            uint32_t len = properties->Length();
            for (uint32_t i = 0; i < len; ++i) {
                v8::Handle<v8::String> keyh = properties->Get(i)->ToString();
                guarantee(!keyh.IsEmpty());
                v8::Handle<v8::Value> valueh = objh->Get(keyh);
                guarantee(!valueh.IsEmpty());

                counted_t<const ql::datum_t> item = js_make_datum(valueh, recursion_limit, errmsg);

                if (!item.has()) {
                    // Result is still empty, the error message has been set
                    return result;
                }

                size_t length = keyh->Utf8Length();
                scoped_array_t<char> temp_buffer(length);
                keyh->WriteUtf8(temp_buffer.data(), length);
                std::string key_string(temp_buffer.data(), length);

                datum_map.insert(std::make_pair(key_string, item));
            }
            result = make_counted<const ql::datum_t>(std::move(datum_map));
        }
    } else if (value->IsNumber()) {
        double num_val = value->NumberValue();

        // so we can use `isfinite` in a GCC 4.4.3-compatible way
        using namespace std; // NOLINT(build/namespaces)
        if (!isfinite(num_val)) {
            errmsg->assign("Number return value is not finite.");
        } else {
            result = make_counted<const ql::datum_t>(num_val);
        }
    } else if (value->IsBoolean()) {
        result = make_counted<const ql::datum_t>(ql::datum_t::R_BOOL, value->BooleanValue());
    } else if (value->IsNull()) {
        result = make_counted<const ql::datum_t>(ql::datum_t::R_NULL);
    } else {
        errmsg->assign(value->IsUndefined() ?
                       "Cannot convert javascript `undefined` to ql::datum_t." :
                       "Unrecognized value type when converting to ql::datum_t.");
    }
    return result;
}

counted_t<const ql::datum_t> js_to_datum(const v8::Handle<v8::Value> &value, std::string *errmsg) {
    guarantee(!value.IsEmpty());
    guarantee(errmsg != NULL);

    DECLARE_HANDLE_SCOPE(handle_scope);
    errmsg->assign("Unknown error when converting to ql::datum_t.");

    return js_make_datum(value, TO_JSON_RECURSION_LIMIT, errmsg);
}

v8::Handle<v8::Value> js_from_datum(const counted_t<const ql::datum_t> &datum) {
    guarantee(datum.has());
    switch (datum->get_type()) {
    case ql::datum_t::type_t::R_BOOL:
        if (datum->as_bool()) {
            return v8::True();
        } else {
            return v8::False();
        }
    case ql::datum_t::type_t::R_NULL:
        return v8::Null();
    case ql::datum_t::type_t::R_NUM:
        return v8::Number::New(datum->as_num());
    case ql::datum_t::type_t::R_STR:
        return v8::String::New(datum->as_str().c_str());
    case ql::datum_t::type_t::R_ARRAY: {
        v8::Handle<v8::Array> array = v8::Array::New();
        const std::vector<counted_t<const ql::datum_t> > &source_array = datum->as_array();

        for (size_t i = 0; i < source_array.size(); ++i) {
            DECLARE_HANDLE_SCOPE(scope);
            v8::Handle<v8::Value> val = js_from_datum(source_array[i]);
            guarantee(!val.IsEmpty());
            array->Set(i, val);
        }

        return array;
    }
    case ql::datum_t::type_t::R_OBJECT: {
        if (datum->is_ptype(ql::pseudo::time_string)) {
            double epoch_time = ql::pseudo::time_to_epoch_time(datum);
            v8::Handle<v8::Value> date = v8::Date::New(epoch_time * 1000);
            return date;
        } else {
            v8::Handle<v8::Object> obj = v8::Object::New();
            const std::map<std::string, counted_t<const ql::datum_t> > &source_map = datum->as_object();

            for (auto it = source_map.begin(); it != source_map.end(); ++it) {
                DECLARE_HANDLE_SCOPE(scope);
                v8::Handle<v8::Value> key = v8::String::New(it->first.c_str());
                v8::Handle<v8::Value> val = js_from_datum(it->second);
                guarantee(!key.IsEmpty() && !val.IsEmpty());
                obj->Set(key, val);
            }

            return obj;
        }
    }

    case ql::datum_t::type_t::UNINITIALIZED:
    default:
        crash("bad datum value in js extproc");
    }
}
