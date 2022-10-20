// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "extproc/js_job.hpp"

// We silence some quickjs unused parameter warnings in inlined functions.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <quickjs/quickjs.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <stdint.h>
#include <limits.h>

#include <algorithm>
#include <exception>
#include <limits>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "extproc/extproc_job.hpp"
#include "logger.hpp"
#include "math.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/serialize_datum.hpp"
#include "utils.hpp"

const js_id_t MIN_ID = 1;

// Picked from a hat.
#define TO_JSON_RECURSION_LIMIT  500

struct quickjs_context;

// Returns an empty datum on error.
ql::datum_t js_to_datum(quickjs_context *ctx,
                        const ql::configured_limits_t &limits,
                        JSValue value,
                        std::string *err_out);

struct quickjs_context {
    unsigned int task_counter;
    JSRuntime *rt;
    JSContext *ctx;
    JSAtom lengthAtom;
    JSAtom valueOfAtom;
    JSAtom ctorDateAtom;
    JSAtom ctorRegExpAtom;
    JSAtom prototypeAtom;
    quickjs_context() : rt(JS_NewRuntime()), ctx(JS_NewContext(rt)) {
        lengthAtom = JS_NewAtom(ctx, "length");
        valueOfAtom = JS_NewAtom(ctx, "valueOf");
        ctorDateAtom = JS_NewAtom(ctx, "Date");
        ctorRegExpAtom = JS_NewAtom(ctx, "RegExp");
        prototypeAtom = JS_NewAtom(ctx, "prototype");
    }
    ~quickjs_context() {
        JS_FreeAtom(ctx, prototypeAtom);
        JS_FreeAtom(ctx, ctorRegExpAtom);
        JS_FreeAtom(ctx, ctorDateAtom);
        JS_FreeAtom(ctx, valueOfAtom);
        JS_FreeAtom(ctx, lengthAtom);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
    }
};

struct quickjs_env {
    quickjs_context *ctx;
    js_id_t next_id = MIN_ID;
    // Each JSValue holds a refcount (which gets freed in the dtor or when it gets
    // removed from the map).
    std::unordered_map<js_id_t, JSValue> values;
    explicit quickjs_env(quickjs_context *_ctx) : ctx(_ctx) {}
    ~quickjs_env() {
        JSContext *jsctx = ctx->ctx;
        for (auto& pair: values) {
            JS_FreeValue(jsctx, pair.second);
        }
    }
};

// TODO: We could use JS_SetInterruptHandler to kill the JS operation
// gracefully, instead of the existing worker process killing
// mechanism.  (The impl using v8 never interrupted the JS engine
// gracefully either.)

std::string qjs_exception_to_string(JSContext *jsctx) {
    JSValue exception = JS_GetException(jsctx);

    size_t len;
    const char *str = JS_ToCStringLen(jsctx, &len, exception);
    guarantee(str != nullptr,
        "Could not convert JS Exception ToString value to string");

    std::string ret(str, len);

    JS_FreeCString(jsctx, str);
    JS_FreeValue(jsctx, exception);
    return ret;
}

js_result_t qjs_eval(quickjs_env *env, const std::string &source,
                     const ql::configured_limits_t &limits) {
    const char *filename = "rethinkdb_query";
    const int eval_flags = JS_EVAL_TYPE_GLOBAL;
    JSContext *jsctx = env->ctx->ctx;
    JSValue value = JS_Eval(jsctx, source.c_str(), source.size(), filename,
        eval_flags);

    js_result_t result("");

    if (JS_IsException(value)) {
        // We set the error message to the exception string value without any prefix.
        // This matches the v8 behavior.
        boost::get<std::string>(result) = qjs_exception_to_string(jsctx);

        JS_FreeValue(jsctx, value);
    } else {
        if (JS_IsFunction(jsctx, value)) {
            js_id_t id = env->next_id;
            ++env->next_id;
            env->values[id] = value;
            result = id;

            // Notably, we don't JS_FreeValue(jsctx, value).
        } else {
            ql::datum_t datum = js_to_datum(env->ctx, limits, value, &boost::get<std::string>(result));
            if (datum.has()) {
                result = std::move(datum);
            }

            JS_FreeValue(jsctx, value);
        }
    }

    return result;
}

bool construct_js_from_datum(quickjs_env *env, const ql::datum_t & datum, JSValue *out, std::string *err_out) {
    guarantee(datum.has());
    JSContext *jsctx = env->ctx->ctx;

    switch (datum.get_type()) {
    case ql::datum_t::type_t::MINVAL:
        err_out->assign("`r.minval` cannot be passed to `r.js`.");
        return false;
    case ql::datum_t::type_t::MAXVAL:
        err_out->assign("`r.maxval` cannot be passed to `r.js`.");
        return false;
    case ql::datum_t::type_t::R_BINARY:
        // TODO: We could support this!  With an ArrayBuffer API?
        err_out->assign("`r.binary` data cannot be used in `r.js`.");
        return false;
    case ql::datum_t::type_t::R_BOOL:
        *out = JS_NewBool(jsctx, datum.as_bool());
        return true;
    case ql::datum_t::type_t::R_NULL:
        *out = JS_NULL;
        return true;
    case ql::datum_t::type_t::R_NUM:
        *out = JS_NewFloat64(jsctx, datum.as_num());
        return true;
    case ql::datum_t::type_t::R_STR: {
        const datum_string_t &str = datum.as_str();
        *out = JS_NewStringLen(jsctx, str.data(), str.size());
    } return true;
    case ql::datum_t::type_t::R_ARRAY: {
        JSValue array = JS_NewArray(jsctx);
        if (JS_IsException(array)) {
            *err_out = "JS exception constructing new array: " + qjs_exception_to_string(jsctx);
            JS_FreeValue(jsctx, array);
            return false;
        }

        rcheck_datum(datum.arr_size() <= size_t(UINT32_MAX),
                     ql::base_exc_t::RESOURCE,
                     "Array over JavaScript size limit `" + std::to_string(UINT32_MAX) + "`.");

        for (size_t i = 0, e = datum.arr_size(); i < e; ++i) {
            ql::datum_t elem = datum.get(i);
            JSValue val;
            if (!construct_js_from_datum(env, elem, &val, err_out)) {
                JS_FreeValue(jsctx, array);
                return false;
            }
            if (JS_DefinePropertyValueUint32(jsctx, array, i, val, JS_PROP_C_W_E) < 0) {
                *err_out = "JS error appending array element";
                JS_FreeValue(jsctx, array);
                return false;
            }
        }

        *out = array;
        return true;
    }
    case ql::datum_t::type_t::R_OBJECT: {
        if (datum.is_ptype(ql::pseudo::time_string)) {
            double epoch_time = ql::pseudo::time_to_epoch_time(datum);

            JSValue globalObject = JS_GetGlobalObject(jsctx);
            JSValue dateCtor = JS_GetProperty(jsctx, globalObject, env->ctx->ctorDateAtom);

            JSValue numberArg[1] = { JS_NewFloat64(jsctx, epoch_time * 1000) };

            JSValue res = JS_CallConstructor(jsctx, dateCtor, 1, numberArg);
            if (JS_IsException(res)) {
                *err_out = "Exception invoking Date constructor: " + qjs_exception_to_string(jsctx);
                // TODO: Use goto's or something for cleanup logic (everywhere).
                JS_FreeValue(jsctx, numberArg[0]);
                JS_FreeValue(jsctx, dateCtor);
                JS_FreeValue(jsctx, globalObject);
                return false;
            }
            *out = res;

            JS_FreeValue(jsctx, numberArg[0]);
            JS_FreeValue(jsctx, dateCtor);
            JS_FreeValue(jsctx, globalObject);
            return true;
        } else {
            JSValue object = JS_NewObject(jsctx);

            for (size_t i = 0, n = datum.obj_size(); i < n; ++i) {
                auto pair = datum.get_pair(i);
                JSValue val;
                if (!construct_js_from_datum(env, pair.second, &val, err_out)) {
                    JS_FreeValue(jsctx, object);
                    return false;
                }
                JSAtom prop = JS_NewAtomLen(jsctx, pair.first.data(), pair.first.size());
                if (JS_DefinePropertyValue(jsctx, object, prop, val, JS_PROP_C_W_E) < 0) {
                    *err_out = "JS error adding object property";
                    JS_FreeAtom(jsctx, prop);
                    JS_FreeValue(jsctx, object);
                    return false;
                }
                JS_FreeAtom(jsctx, prop);
            }

            *out = object;
            return true;
        }
    }
    case ql::datum_t::type_t::UNINITIALIZED: // fallthru
    default:
        err_out->assign("bad datum value in js extproc");
        return false;
    }
}

js_result_t qjs_call(quickjs_env *env, js_id_t id, const std::vector<ql::datum_t> &args, const ql::configured_limits_t &limits) {
    js_result_t result("");
    JSContext *jsctx = env->ctx->ctx;

    std::vector<JSValue> jsArgs;
    for (const ql::datum_t &datum : args) {
        jsArgs.emplace_back();
        if (!construct_js_from_datum(env, datum, &jsArgs.back(), &boost::get<std::string>(result))) {
            for (JSValue val : jsArgs) {
                JS_FreeValue(jsctx, val);
            }
            return result;
        }
    }

    size_t jsArgsSize = jsArgs.size();
    if (jsArgsSize > INT_MAX) {
        boost::get<std::string>(result) = "JavaScript function invoked with too many arguments";
        for (JSValue val : jsArgs) {
            JS_FreeValue(jsctx, val);
        }
        return result;
    }

    auto func_it = env->values.find(id);
    guarantee(func_it != env->values.end(), "js_id_t not found");
    JSValue func_obj = func_it->second;

    JSValue globalObject = JS_GetGlobalObject(jsctx);
    JSValue value = JS_Call(jsctx, func_obj, globalObject, jsArgsSize,
                            reinterpret_cast<JSValueConst *>(jsArgs.data()));
    JS_FreeValue(jsctx, globalObject);

    for (JSValue val : jsArgs) {
        JS_FreeValue(jsctx, val);
    }

    if (JS_IsException(value)) {
        // We set the error message to the exception string value without any prefix.
        // This matches the v8 behavior.
        boost::get<std::string>(result) = qjs_exception_to_string(jsctx);
    } else {
        if (JS_IsFunction(jsctx, value)) {
            boost::get<std::string>(result) = "Returning functions from within `r.js` is unsupported.";
        } else {
            ql::datum_t datum = js_to_datum(env->ctx, limits, value, &boost::get<std::string>(result));
            if (datum.has()) {
                result = std::move(datum);
            }
        }
    }

    JS_FreeValue(jsctx, value);

    return result;
}

bool qjs_is_of_type(quickjs_context *ctx, JSValue value, JSAtom typ) {
    JSContext *jsctx = ctx->ctx;
    JSValue prototype = JS_GetPrototype(jsctx, value);

    JSValue globalObject = JS_GetGlobalObject(jsctx);

    JSValue jsType = JS_GetProperty(jsctx, globalObject, typ);
    JSValue jsTypePrototype = JS_GetProperty(jsctx, jsType, ctx->prototypeAtom);

    bool ret = false;

    bool prototypeIsObject = JS_IsObject(prototype);
    bool jsTypePrototypeIsObject = JS_IsObject(jsTypePrototype);

    if (prototypeIsObject && jsTypePrototypeIsObject) {
        void *p1 = JS_VALUE_GET_PTR(prototype);
        void *p2 = JS_VALUE_GET_PTR(jsTypePrototype);

        ret = p1 == p2;
    }

    JS_FreeValue(jsctx, jsTypePrototype);
    JS_FreeValue(jsctx, jsType);
    JS_FreeValue(jsctx, globalObject);
    JS_FreeValue(jsctx, prototype);

    return ret;
}

// TODO: We could support binary and bigint types.

ql::datum_t js_make_datum(quickjs_context *qjs_ctx,
                          int recursion_limit,
                          const ql::configured_limits_t &limits,
                          JSValue value,
                          std::string *err_out) {
    JSContext *const ctx = qjs_ctx->ctx;
    ql::datum_t result;

    if (0 == recursion_limit) {
        err_out->assign("Recursion limit exceeded in js_to_json (circular reference?).");
        return result;
    }
    --recursion_limit;

    // TODO: Use JS_VALUE_GET_TAG and a switch (after testing)
    if (JS_IsString(value)) {
        size_t len;
        const char *str = JS_ToCStringLen(ctx, &len, value);
        guarantee(str != nullptr,
            "JS_IsString was true, but JS_ToCStringLen returned nullptr");
        result = ql::datum_t(datum_string_t(len, str));
        JS_FreeCString(ctx, str);
    } else if (JS_IsObject(value)) {
        if (JS_IsArray(ctx, value)) {
            JSValue length_value = JS_GetProperty(ctx, value, qjs_ctx->lengthAtom);

            // In JavaScript, arrays can have length up to 2**32-1.
            uint32_t length32;
            if (!JS_IsNumber(length_value)) {
                err_out->assign("Array length is not a number");
            } else if (0 != JS_ToUint32(ctx, &length32, length_value)) {
                // Quickjs JS_ToUint32 actually converts non-integral floats to integers.
                // As well as negative values.
                err_out->assign("Array length is not convertable to a small integer");
            } else {
                rcheck_array_size_value_datum(length32, limits);
                std::vector<ql::datum_t> datum_array;
                datum_array.reserve(length32);
                for (uint32_t i = 0; i < length32; ++i) {
                    JSValue elem = JS_GetPropertyUint32(ctx, value, i);
                    ql::datum_t item = js_make_datum(qjs_ctx, recursion_limit, limits, elem, err_out);
                    if (!item.has()) {
                        // Result is still empty, the error message has been set
                        JS_FreeValue(ctx, elem);
                        return result;
                    }
                    datum_array.push_back(std::move(item));
                    JS_FreeValue(ctx, elem);
                }

                result = ql::datum_t(std::move(datum_array), limits);
            }

            JS_FreeValue(ctx, length_value);
        } else if (JS_IsFunction(ctx, value)) {
            err_out->assign("Cannot convert function to ql::datum_t.");
        } else if (qjs_is_of_type(qjs_ctx, value, qjs_ctx->ctorRegExpAtom)) {
            err_out->assign("Cannot convert RegExp to ql::datum_t.");
        } else if (qjs_is_of_type(qjs_ctx, value, qjs_ctx->ctorDateAtom)) {
            JSValueConst argv[1];
            JSValue valueOf = JS_Invoke(ctx, value, qjs_ctx->valueOfAtom, 0, argv);

            double floatValue;
            if (0 != JS_ToFloat64(ctx, &floatValue, valueOf)) {
                err_out->assign("Invalid Date in JS environment");
            } else {
                result = ql::pseudo::make_time(floatValue * (1.0 / 1000.0), "+00:00");
            }
            JS_FreeValue(ctx, valueOf);
        } else {
            ql::datum_object_builder_t builder;

            JSPropertyEnum *ptab;
            uint32_t plen;

            // We only get string properties.  The original v8 implementation didn't
            // have symbol properties (and perhaps not even private properties).
            if (0 != JS_GetOwnPropertyNames(ctx, &ptab, &plen, value, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY)) {
                err_out->assign("Could not get object properties.");
            } else {
                for (uint32_t i = 0; i < plen; ++i) {
                    JSAtom atom = ptab[i].atom;
                    JSValue elem = JS_GetProperty(ctx, value, atom);

                    ql::datum_t item = js_make_datum(qjs_ctx, recursion_limit, limits, elem, err_out);

                    if (!item.has()) {
                        // Result is still empty, the error message has been set
                        JS_FreeAtom(ctx, atom);
                        JS_FreeValue(ctx, elem);
                        return result;
                    }

                    const char *atom_str = JS_AtomToCString(ctx, atom);
                    if (atom_str == nullptr) {
                        err_out->assign("Error converting JSAtom to string");
                        JS_FreeAtom(ctx, atom);
                        JS_FreeValue(ctx, elem);
                        // Result is still empty.
                        return result;
                    }

                    datum_string_t key_string(atom_str);
                    JS_FreeCString(ctx, atom_str);

                    builder.overwrite(key_string, std::move(item));

                    JS_FreeAtom(ctx, atom);
                    JS_FreeValue(ctx, elem);
                }
                js_free(ctx, ptab);

                result = std::move(builder).to_datum();
            }
        }
    } else if (JS_IsNumber(value)) {
        double num_val;
        int res = JS_ToFloat64(ctx, &num_val, value);
        guarantee(res == 0, "JS_IsNumber was true but JS_ToFloat64 failed");
        if (!risfinite(num_val)) {
            err_out->assign("Number return value is not finite.");
        } else {
            result = ql::datum_t(num_val);
        }
    } else if (JS_IsBool(value)) {
        int res = JS_ToBool(ctx, value);
        guarantee(res != -1, "JS_IsBool returned true, but we have an exception");
        result = ql::datum_t::boolean(static_cast<bool>(res));
    } else if (JS_IsNull(value)) {
        // Duktape had a bug where a null JS value produced an error.  The original v8
        // implementation converted it to a null datum.  As do we.
        result = ql::datum_t::null();
    } else if (JS_IsUndefined(value)) {
        // The v8 engine did have this error message
        err_out->assign("Cannot convert javascript `undefined` to ql::datum_t.");
    } else {
        err_out->assign("Unhandled value type when converting to ql::datum_t.");
    }

    return result;
}

void qjs_release(quickjs_env *env, js_id_t id) {
    auto it = env->values.find(id);
    guarantee(it != env->values.end(),
        "js_id_t value (function identifier) not found when releasing function");
    JSValue value = it->second;
    env->values.erase(it);
    JS_FreeValue(env->ctx->ctx, value);
}

enum js_task_t {
    TASK_EVAL,
    TASK_CALL,
    TASK_RELEASE,
    TASK_EXIT
};

// The job_t runs in the context of the main rethinkdb process
js_job_t::js_job_t(extproc_pool_t *pool, signal_t *interruptor,
                   const ql::configured_limits_t &_limits) :
    extproc_job(pool, &worker_fn, interruptor), limits(_limits) { }

js_result_t js_job_t::eval(const std::string &source) {
    js_task_t task = js_task_t::TASK_EVAL;
    write_message_t wm;
    wm.append(&task, sizeof(task));
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, source);
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, limits);
    {
        int res = send_write_message(extproc_job.write_stream(), &wm);
        if (res != 0) {
            throw extproc_worker_exc_t("failed to send data to the worker");
        }
    }

    js_result_t result;
    archive_result_t res
        = deserialize<cluster_version_t::LATEST_OVERALL>(extproc_job.read_stream(),
                                                         &result);
    if (bad(res)) {
        throw extproc_worker_exc_t(strprintf("failed to deserialize eval result from worker "
                                             "(%s)", archive_result_as_str(res)));
    }
    return result;
}

js_result_t js_job_t::call(js_id_t id, const std::vector<ql::datum_t> &args) {
    js_task_t task = js_task_t::TASK_CALL;
    write_message_t wm;
    wm.append(&task, sizeof(task));
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, id);
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, args);
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, limits);
    {
        int res = send_write_message(extproc_job.write_stream(), &wm);
        if (res != 0) {
            throw extproc_worker_exc_t("failed to send data to the worker");
        }
    }

    js_result_t result;
    archive_result_t res
        = deserialize<cluster_version_t::LATEST_OVERALL>(extproc_job.read_stream(),
                                                         &result);
    if (bad(res)) {
        throw extproc_worker_exc_t(strprintf("failed to deserialize call result from worker "
                                             "(%s)", archive_result_as_str(res)));
    }
    return result;
}

void js_job_t::release(js_id_t id) {
    js_task_t task = js_task_t::TASK_RELEASE;
    write_message_t wm;
    wm.append(&task, sizeof(task));
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, id);
    {
        int res = send_write_message(extproc_job.write_stream(), &wm);
        if (res != 0) {
            throw extproc_worker_exc_t("failed to send data to the worker");
        }
    }

    // Wait for a response so we don't flood the job with requests
    bool dummy_result;
    archive_result_t res
        = deserialize<cluster_version_t::LATEST_OVERALL>(extproc_job.read_stream(),
                                                         &dummy_result);
    // dummy_result should always be true
    if (bad(res) || !dummy_result) {
        throw extproc_worker_exc_t(strprintf("failed to deserialize release result from worker "
                                             "(%s)", archive_result_as_str(res)));
    }
}

void js_job_t::exit() {
    js_task_t task = js_task_t::TASK_EXIT;
    write_message_t wm;
    wm.append(&task, sizeof(task));
    {
        int res = send_write_message(extproc_job.write_stream(), &wm);
        if (res != 0) {
            throw extproc_worker_exc_t("failed to send data to the worker");
        }
    }

    // Wait for a response so we don't flood the job with requests
    bool dummy_result;
    archive_result_t res
        = deserialize<cluster_version_t::LATEST_OVERALL>(extproc_job.read_stream(),
                                                         &dummy_result);
    // dummy_result should always be true
    if (bad(res) || !dummy_result) {
        throw extproc_worker_exc_t(strprintf("failed to deserialize exit result from worker "
                                             "(%s)", archive_result_as_str(res)));
    }
}

void js_job_t::worker_error() {
    extproc_job.worker_error();
}

bool send_js_result(write_stream_t *stream_out, const js_result_t &js_result) {
    write_message_t wm;
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, js_result);
    int res = send_write_message(stream_out, &wm);
    return res == 0;
}

bool send_dummy_result(write_stream_t *stream_out) {
    write_message_t wm;
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, true);
    int res = send_write_message(stream_out, &wm);
    return res == 0;
}

void run_other_tasks(quickjs_env *env) {
    if ((env->ctx->task_counter & 127) == 0) {
        JS_RunGC(env->ctx->rt);
    }
}

bool run_eval(read_stream_t *stream_in,
              write_stream_t *stream_out,
              quickjs_env *qjs_env) {
    std::string source;
    ql::configured_limits_t limits;
    {
        archive_result_t res
            = deserialize<cluster_version_t::LATEST_OVERALL>(stream_in,
                                                             &source);
        if (bad(res)) { return false; }
        res = deserialize<cluster_version_t::LATEST_OVERALL>(stream_in,
                                                             &limits);
        if (bad(res)) { return false; }
    }

    js_result_t js_result;
    try {
        js_result = qjs_eval(qjs_env, source, limits);
    } catch (const std::exception &e) {
        js_result = e.what();
    } catch (...) {
        js_result = std::string("encountered an unknown exception");
    }

    run_other_tasks(qjs_env);
    return send_js_result(stream_out, js_result);
}

bool run_call(read_stream_t *stream_in,
              write_stream_t *stream_out,
              quickjs_env *env) {
    js_id_t id;
    std::vector<ql::datum_t> args;
    ql::configured_limits_t limits;
    {
        archive_result_t res
            = deserialize<cluster_version_t::LATEST_OVERALL>(stream_in, &id);
        if (bad(res)) { return false; }
        res = deserialize<cluster_version_t::LATEST_OVERALL>(stream_in, &args);
        if (bad(res)) { return false; }
        res = deserialize<cluster_version_t::LATEST_OVERALL>(stream_in, &limits);
        if (bad(res)) { return false; }
    }

    js_result_t js_result;
    try {
        js_result = qjs_call(env, id, std::move(args), limits);
    } catch (const std::exception &e) {
        js_result = e.what();
    } catch (...) {
        js_result = std::string("encountered an unknown exception");
    }

    run_other_tasks(env);
    return send_js_result(stream_out, js_result);
}

bool run_release(read_stream_t *stream_in,
                 write_stream_t *stream_out,
                 quickjs_env *env) {
    js_id_t id;
    archive_result_t res = deserialize<cluster_version_t::LATEST_OVERALL>(stream_in,
                                                                          &id);
    if (bad(res)) { return false; }

    qjs_release(env, id);
    run_other_tasks(env);
    return send_dummy_result(stream_out);
}

bool run_exit(write_stream_t *stream_out) {
    return send_dummy_result(stream_out);
}

bool js_job_t::worker_fn(read_stream_t *stream_in, write_stream_t *stream_out) {
    bool running = true;

    // We could combine these two objects quickjs_context and
    // quickjs_env.  With previous engines, some "runtime/global
    // context" value was held in a global variable.
    quickjs_context context;
    quickjs_env qjs_env(&context);

    while (running) {
        context.task_counter += 1;
        js_task_t task;
        int64_t read_size = sizeof(task);
        {
            int64_t res = force_read(stream_in, &task, read_size);
            if (res != read_size) { return false; }
        }

        switch (task) {
        case TASK_EVAL:
            if (!run_eval(stream_in, stream_out, &qjs_env)) {
                return false;
            }
            break;
        case TASK_CALL:
            if (!run_call(stream_in, stream_out, &qjs_env)) {
                return false;
            }
            break;
        case TASK_RELEASE:
            if (!run_release(stream_in, stream_out, &qjs_env)) {
                return false;
            }
            break;
        case TASK_EXIT:
            return run_exit(stream_out);
        default:
            return false;
        }
    }
    unreachable();
}

ql::datum_t js_to_datum(quickjs_context *ctx,
                        const ql::configured_limits_t &limits,
                        JSValue value,
                        std::string *err_out) {
    return js_make_datum(ctx, TO_JSON_RECURSION_LIMIT, limits, value, err_out);
}

