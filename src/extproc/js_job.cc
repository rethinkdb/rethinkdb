// Copyright 2010-2013 RethinkDB, all rights reserved.

#define __STDC_LIMIT_MACROS
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

#include "extproc/js_job.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "containers/archive/boost_types.hpp"
#include "extproc/extproc_job.hpp"

const js_id_t MIN_ID = 1;
const js_id_t MAX_ID = UINT64_MAX;

// Picked from a hat.
#define TO_JSON_RECURSION_LIMIT  500

// Returns an empty pointer on error.
boost::shared_ptr<scoped_cJSON_t> js_to_json(const v8::Handle<v8::Value> &value,
                                         std::string *errmsg);

// Should never error.
v8::Handle<v8::Value> js_from_json(const cJSON &json);

// Worker-side JS evaluation environment.
class js_env_t {
public:
    js_env_t();
    ~js_env_t();

    js_result_t eval(const std::string &source);
    js_result_t call(js_id_t id, const std::vector<boost::shared_ptr<scoped_cJSON_t> > &args);
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
    write_message_t msg;
    msg.append(&task, sizeof(task));
    msg << source;
    int res = send_write_message(extproc_job.write_stream(), &msg);
    if (res != 0) { throw js_worker_exc_t("failed to send data to the worker"); }

    js_result_t result;
    res = deserialize(extproc_job.read_stream(), &result);
    if (res != ARCHIVE_SUCCESS) { throw js_worker_exc_t("failed to deserialize result from worker"); }
    return result;
}

js_result_t js_job_t::call(js_id_t id, std::vector<boost::shared_ptr<scoped_cJSON_t> > args) {
    js_task_t task = js_task_t::TASK_CALL;
    write_message_t msg;
    msg.append(&task, sizeof(task));
    msg << id;
    msg << args;
    int res = send_write_message(extproc_job.write_stream(), &msg);
    if (res != 0) { throw js_worker_exc_t("failed to send data to the worker"); }

    js_result_t result;
    res = deserialize(extproc_job.read_stream(), &result);
    if (res != ARCHIVE_SUCCESS) { throw js_worker_exc_t("failed to deserialize result from worker"); }
    return result;
}

void js_job_t::release(js_id_t id) {
    js_task_t task = js_task_t::TASK_RELEASE;
    write_message_t msg;
    msg.append(&task, sizeof(task));
    msg << id;
    int res = send_write_message(extproc_job.write_stream(), &msg);
    if (res != 0) { throw js_worker_exc_t("failed to send data to the worker"); }
}

void js_job_t::exit() {
    js_task_t task = js_task_t::TASK_EXIT;
    write_message_t msg;
    msg.append(&task, sizeof(task));
    int res = send_write_message(extproc_job.write_stream(), &msg);
    if (res != 0) { throw js_worker_exc_t("failed to send data to the worker"); }
}

bool js_job_t::worker_fn(read_stream_t *stream_in, write_stream_t *stream_out) {
    bool running = true;
    js_env_t js_env;

    while (running) {
        js_task_t task;
        int64_t read_size = sizeof(task);
        int64_t res = force_read(stream_in, &task, read_size);
        if (res != read_size) { return false; }

        switch (task) {
        case TASK_EVAL:
            {
                std::string source;
                res = deserialize(stream_in, &source);
                if (res != ARCHIVE_SUCCESS) { return false; }

                js_result_t js_result = js_env.eval(source);
                write_message_t msg;
                msg << js_result;
                res = send_write_message(stream_out, &msg);
                if (res != 0) { return false; }
            }
            break;
        case TASK_CALL:
            {
                js_id_t id;
                std::vector<boost::shared_ptr<scoped_cJSON_t> > args;
                res = deserialize(stream_in, &id);
                if (res != ARCHIVE_SUCCESS) { return false; }
                res = deserialize(stream_in, &args);
                if (res != ARCHIVE_SUCCESS) { return false; }

                js_result_t js_result = js_env.call(id, args);
                write_message_t msg;
                msg << js_result;
                res = send_write_message(stream_out, &msg);
                if (res != 0) { return false; }
            }
            break;
        case TASK_RELEASE:
            {
                js_id_t id;
                res = deserialize(stream_in, &id);
                if (res != ARCHIVE_SUCCESS) { return false; }
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

    v8::HandleScope handle_scope;

    // TODO(rntz): use an "external resource" to avoid copy?
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
                boost::shared_ptr<scoped_cJSON_t> json = js_to_json(result_val, errmsg);
                if (json) {
                    result = json;
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
                                  const std::vector<boost::shared_ptr<scoped_cJSON_t> > &args,
                                  std::string *errmsg) {
    v8::TryCatch try_catch;
    v8::HandleScope scope;

    // Construct receiver object.
    v8::Handle<v8::Object> obj = v8::Object::New();
    guarantee(!obj.IsEmpty());

    // Construct arguments.
    scoped_array_t<v8::Handle<v8::Value> > handles(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        handles[i] = js_from_json(*args[i]->get());
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
                           const std::vector<boost::shared_ptr<scoped_cJSON_t> > &args) {
    js_context_t clean_context;
    js_result_t result("");
    std::string *errmsg = boost::get<std::string>(&result);

    const boost::shared_ptr<v8::Persistent<v8::Value> > found_value = find_value(id);
    guarantee(!found_value->IsEmpty());

    v8::HandleScope handle_scope;

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
            boost::shared_ptr<scoped_cJSON_t> json = js_to_json(value, errmsg);
            if (json) {
                result = json;
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

// Returns NULL & sets `*errmsg` on failure.
//
// TODO(rntz): Is there a better way of detecting cyclic data structures than
// using a recursion limit?
static cJSON *js_make_json(const v8::Handle<v8::Value> &value, int recursion_limit, std::string *errmsg) {
    if (0 == recursion_limit) {
        *errmsg = "toJSON recursion limit exceeded (cyclic datastructure?)";
        return NULL;
    }
    --recursion_limit;

    // TODO(rntz): should we handle BooleanObject, NumberObject, StringObject?
    v8::HandleScope handle_scope;

    if (value->IsString()) {
        cJSON *p = cJSON_CreateBlank();
        if (NULL == p) {
            *errmsg = "cJSON_CreateBlank() failed";
            return NULL;
        }
        scoped_cJSON_t result(p);

        // Copy in the string. TODO(rntz): cJSON requires null termination. We
        // should switch away from cJSON.
        v8::Handle<v8::String> string = value->ToString();
        guarantee(!string.IsEmpty());
        int length = string->Utf8Length() + 1; // +1 for null byte

        p->type = cJSON_String;
        p->valuestring = reinterpret_cast<char *>(malloc(length));
        if (NULL == p->valuestring) {
            *errmsg = "failed to allocate space for string";
            return NULL;
        }
        string->WriteUtf8(p->valuestring, length);

        return result.release();

    } else if (value->IsObject()) {
        // This case is kinda weird. Objects can have stuff in them that isn't
        // represented in their JSON (eg. their prototype, v8 hidden fields).

        if (value->IsArray()) {
            v8::Handle<v8::Array> arrayh = v8::Handle<v8::Array>::Cast(value);
            scoped_cJSON_t arrayj(cJSON_CreateArray());
            if (NULL == arrayj.get()) {
                *errmsg = "cJSON_CreateArray() failed";
                return NULL;
            }

            uint32_t len = arrayh->Length();
            for (uint32_t i = 0; i < len; ++i) {
                v8::Handle<v8::Value> elth = arrayh->Get(i);
                guarantee(!elth.IsEmpty()); // FIXME

                cJSON *eltj = js_make_json(elth, recursion_limit, errmsg);
                if (NULL == eltj) return NULL;

                // Append it to the array.
                cJSON_AddItemToArray(arrayj.get(), eltj);
            }

            return arrayj.release();

        } else if (value->IsFunction()) {
            // We can't represent functions in JSON.
            *errmsg = "Can't convert function to JSON";
            return NULL;

        } else if (value->IsRegExp()) {
            // Ditto.
            *errmsg = "Can't convert RegExp to JSON";
            return NULL;

        } else {
            // Treat it as a dictionary.
            v8::Handle<v8::Object> objh = value->ToObject();
            guarantee(!objh.IsEmpty()); // FIXME
            v8::Handle<v8::Array> props = objh->GetPropertyNames();
            guarantee(!props.IsEmpty()); // FIXME

            scoped_cJSON_t objj(cJSON_CreateObject());
            if (NULL == objj.get()) {
                *errmsg = "cJSON_CreateObject() failed";
                return NULL;
            }

            uint32_t len = props->Length();
            for (uint32_t i = 0; i < len; ++i) {
                v8::Handle<v8::String> keyh = props->Get(i)->ToString();
                guarantee(!keyh.IsEmpty()); // FIXME
                v8::Handle<v8::Value> valueh = objh->Get(keyh);
                guarantee(!valueh.IsEmpty()); // FIXME

                scoped_cJSON_t valuej(js_make_json(valueh, recursion_limit, errmsg));
                if (NULL == valuej.get()) return NULL;

                // Create string key.
                int length = keyh->Utf8Length() + 1; // +1 for null byte.
                char *str = valuej.get()->string = reinterpret_cast<char *>(malloc(length));
                if (NULL == str) {
                    *errmsg = "could not allocate space for string";
                    return NULL;
                }
                keyh->WriteUtf8(str, length);

                // Append to object.
                cJSON_AddItemToArray(objj.get(), valuej.release());
            }

            return objj.release();
        }

    } else if (value->IsNumber()) {
        double d = value->NumberValue();
        cJSON *r = NULL;
        // so we can use `isfinite` in a GCC 4.4.3-compatible way
        using namespace std;  // NOLINT(build/namespaces)
        if (isfinite(d)) {
            r = cJSON_CreateNumber(value->NumberValue());
        }
        if (r == NULL) {
            *errmsg = "cJSON_CreateNumber() failed";
        }

        return r;
    } else if (value->IsBoolean()) {
        cJSON *r = cJSON_CreateBool(value->BooleanValue());
        if (r == NULL) {
            *errmsg = "cJSON_CreateBool() failed";
        }

        return r;
    } else if (value->IsNull()) {
        cJSON *r = cJSON_CreateNull();
        if (r == NULL) {
            *errmsg = "cJSON_CreateNull() failed";
        }

        return r;
    } else {
        *errmsg = value->IsUndefined()
            ? "Cannot convert javascript `undefined` to JSON."
            : "Unrecognized value type when converting to JSON.";
        return NULL;
    }
}

boost::shared_ptr<scoped_cJSON_t> js_to_json(const v8::Handle<v8::Value> &value, std::string *errmsg) {
    guarantee(!value.IsEmpty());
    guarantee(errmsg);

    // TODO (rntz): probably want a TryCatch for javascript errors that might happen.
    v8::HandleScope handle_scope;
    *errmsg = "unknown error when converting to JSON";

    cJSON *json = js_make_json(value, TO_JSON_RECURSION_LIMIT, errmsg);
    if (json) {
        return boost::make_shared<scoped_cJSON_t>(json);
    } else {
        return boost::shared_ptr<scoped_cJSON_t>();
    }
}

v8::Handle<v8::Value> js_from_json(const cJSON &json) {
    switch (json.type & ~cJSON_IsReference) {
    case cJSON_False:
        return v8::False();
    case cJSON_True:
        return v8::True();
    case cJSON_NULL:
        return v8::Null();
    case cJSON_Number:
        return v8::Number::New(json.valuedouble);
    case cJSON_String:
        return v8::String::New(json.valuestring);
    case cJSON_Array: {
        v8::Handle<v8::Array> array = v8::Array::New();

        uint32_t index = 0;
        for (cJSON *head = json.head; head; head = head->next, ++index) {
            v8::HandleScope scope;
            v8::Handle<v8::Value> val = js_from_json(*head);
            guarantee(!val.IsEmpty());
            array->Set(index, val);
            // FIXME: try_catch code
        }

        return array;
    }
    case cJSON_Object: {
        v8::Handle<v8::Object> obj = v8::Object::New();

        for (cJSON *head = json.head; head; head = head->next) {
            v8::HandleScope scope;
            v8::Handle<v8::Value> key = v8::String::New(head->string);
            v8::Handle<v8::Value> val = js_from_json(*head);
            guarantee(!key.IsEmpty() && !val.IsEmpty());

            obj->Set(key, val); // FIXME: try_catch code
        }

        return obj;
    }

    default:
        crash("bad cJSON value");
    }
}
