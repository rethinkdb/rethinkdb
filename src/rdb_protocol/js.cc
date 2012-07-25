#define __STDC_LIMIT_MACROS     // hack. :(
#include <stdint.h>
#define MAX_ID UINT32_MAX

#include "utils.hpp"
#include <boost/make_shared.hpp>

#include "rdb_protocol/jsimpl.hpp"

namespace js {

// ---------- Utility functions ----------
// See also rdb_protocol/tofromjson.cc

v8::Handle<v8::Value> eval(const std::string &srcstr, std::string *errmsg) {
    v8::HandleScope scope;
    v8::Handle<v8::Value> result;

    // TODO (rntz): utf-8 source code support
    v8::Handle<v8::String> src = v8::String::New(srcstr.c_str());

    v8::TryCatch try_catch;

    v8::Handle<v8::Script> script = v8::Script::Compile(src);
    if (script.IsEmpty()) {
        // TODO (rntz): use try_catch for error message
        *errmsg = "script compilation failed";
        // FIXME: do we need to close over an empty handle?
        return result;
    }

    result = script->Run();
    if (result.IsEmpty()) {
        // TODO (rntz): use try_catch for error message
        *errmsg = "script execution failed";
        // FIXME: do we need to close over an empty handle?
        return result;
    }

    return scope.Close(result);
}


// ---------- handle_t & subclasses ----------
handle_t::~handle_t() {
    if (!empty()) release();
}

void handle_t::release() {
    guarantee(!empty());
    parent_->release_id(id_);
    parent_ = NULL;
}


// ---------- env_t ----------
runner_t::req_config_t::req_config_t()
    : timeout(0)
{}

env_t::env_t(extproc::job_t::control_t *control)
    : control_(control),
      should_quit_(false)
{}

env_t::~env_t() {
    // Clean up handles.
    for (std::map<id_t, v8::Persistent<v8::Value> >::iterator it = values_.begin();
         it != values_.end();
         ++it)
        it->second.Dispose();

    for (std::map<id_t, v8::Persistent<v8::ObjectTemplate> >::iterator it = templates_.begin();
         it != templates_.end();
         ++it)
        it->second.Dispose();
}

void env_t::run() {
    while (!should_quit_) {
        guarantee(-1 != extproc::job_t::accept_job(control_, this));
    }
}

void env_t::shutdown() { should_quit_ = true; }

id_t env_t::rememberValue(v8::Handle<v8::Value> value) {
    id_t id = new_id();
    values_.insert(std::make_pair(id, v8::Persistent<v8::Value>::New(value)));
    return id;
}

id_t env_t::rememberTemplate(v8::Handle<v8::ObjectTemplate> templ) {
    id_t id = new_id();
    templates_.insert(std::make_pair(id, v8::Persistent<v8::ObjectTemplate>::New(templ)));
    return id;
}

v8::Handle<v8::Value> env_t::findValue(id_t id) {
    std::map<id_t, v8::Persistent<v8::Value> >::iterator it = values_.find(id);
    guarantee(it != values_.end());
    return it->second;
}

v8::Handle<v8::ObjectTemplate> env_t::findTemplate(id_t id) {
    std::map<id_t, v8::Persistent<v8::ObjectTemplate> >::iterator it = templates_.find(id);
    guarantee(it != templates_.end());
    return it->second;
}

id_t env_t::new_id() {
    guarantee(next_id_ < MAX_ID); // overflow would be bad
    return next_id_++;
}

void env_t::forget(id_t id) {
    guarantee(id < next_id_);
    guarantee(1 == values_.erase(id) + templates_.erase(id));
}


// ---------- runner_t ----------
runner_t::runner_t()
    : DEBUG_ONLY(running_task_(false))
{}

// TODO (rntz): should we check that we have no used ids? (ie. no remaining
// handles?)
runner_t::~runner_t() {}

void runner_t::begin(extproc::pool_t *pool) {
    // TODO(rntz): might eventually want to handle external process failure
    guarantee(0 == extproc::job_handle_t::begin(pool, job_t()));
}

void runner_t::interrupt() {
    extproc::job_handle_t::interrupt();
}

struct quit_task_t : auto_task_t<quit_task_t> {
    RDB_MAKE_ME_SERIALIZABLE_0();
    void run(env_t *env) { env->shutdown(); }
};

void runner_t::finish() {
    rassert(connected());
    run_task_t(this, quit_task_t());
    extproc::job_handle_t::release();
}

void runner_t::create_handle(handle_t *handle, id_t id) {
    rassert(handle->empty());
    handle->parent_ = this;
    handle->id_ = id;
    on_new_id(id);
}

void runner_t::job_t::run_job(control_t *control, UNUSED void *extra) {
    // The reason we have env_t is to use it here.
    env_t(control).run();
}

runner_t::run_task_t::run_task_t(runner_t *runner, const task_t &task)
    : runner_(runner)
{
    rassert(!runner_->running_task_);
    DEBUG_ONLY_CODE(runner_->running_task_ = true);
    guarantee(0 == task.send_over(runner_));
}

runner_t::run_task_t::~run_task_t() {
    rassert(runner_->running_task_);
    DEBUG_ONLY_CODE(runner_->running_task_ = false);
}

#ifndef NDEBUG
void runner_t::on_new_id(id_t id) {
    rassert(connected());
    rassert(!used_ids_.count(id));
    used_ids_.insert(id);
}
#endif


// ---------- tasks ----------
// ----- release_id() -----
struct release_task_t : auto_task_t<release_task_t> {
    release_task_t() {}
    explicit release_task_t(id_t id) : id_(id) {}
    id_t id_;
    RDB_MAKE_ME_SERIALIZABLE_1(id_);
    void run(env_t *env) {
        env->forget(id_);
    }
};

void runner_t::release_id(id_t id) {
    rassert(connected());
    rassert(used_ids_.count(id));

    run_task_t(this, release_task_t(id));

    DEBUG_ONLY_CODE(used_ids_.erase(id));
}

// ----- eval() -----
struct eval_task_t : auto_task_t<eval_task_t> {
    eval_task_t() {}
    eval_task_t(const std::map<std::string, boost::shared_ptr<scoped_cJSON_t> > &env,
                const std::string src)
        : env_(env), src_(src) {}

    std::map<std::string, boost::shared_ptr<scoped_cJSON_t> > env_;
    std::string src_;
    RDB_MAKE_ME_SERIALIZABLE_2(env_, src_);

    v8::Handle<v8::Value> eval(std::string *errmsg) {
        v8::HandleScope scope;
        v8::Handle<v8::Value> result;      // empty initially.

        // Compile & run script to get a function.
        // TODO (rntz): should do this mangling engine-side, or even client-side.
        std::string func_src_ = strprintf("(function(){%s})", src_.c_str());
        // TODO(rntz): use an "external resource" to avoid copy
        v8::Handle<v8::String> src = v8::String::New(func_src_.c_str());

        v8::TryCatch try_catch;

        v8::Handle<v8::Script> script = v8::Script::Compile(src);
        if (script.IsEmpty()) {
            // TODO (rntz): use try_catch for error message
            *errmsg = "compiling function definition failed";
            // FIXME: do we need to close over an empty handle?
            return result;
        }

        v8::Handle<v8::Value> funcv = script->Run();
        if (funcv.IsEmpty()) {
            // TODO (rntz): use try_catch for error message
            *errmsg = "evaluating function definition failed";
            // FIXME: do we need to close over an empty handle?
            return result;
        }
        if (!funcv->IsFunction()) {
            *errmsg = "evaluating function definition did not produce function";
            // FIXME: do we need to close over an empty handle?
            return result;
        }
        v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(funcv);
        rassert(!func.IsEmpty());

        // Construct environment.
        v8::Handle<v8::Object> ctx = v8::Object::New();
        for (std::map<std::string, boost::shared_ptr<scoped_cJSON_t> >::iterator it = env_.begin();
             it != env_.end();
             ++it)
        {
            // TODO(rntz): use an "external resource" to avoid copy
            v8::Handle<v8::Value> key = v8::String::New(it->first.c_str());
            v8::Handle<v8::Value> val = fromJSON(*it->second.get()->get());
            rassert(!key.IsEmpty() && !val.IsEmpty());

            // TODO(rntz): figure out what the difference between Set() and
            // ForceSet() is.
            ctx->Set(key, val);
            // FIXME: check try_catch?
        }

        // Call function with environment as its receiver.
        result = func->Call(ctx, 0, NULL);
        if (result.IsEmpty()) {
            // TODO (rntz): use try_catch for error message
            *errmsg = "calling function failed";
            if (try_catch.HasCaught()) {
                v8::Handle<v8::String> msg = try_catch.Message()->Get();

                // FIXME: overflow problem.
                size_t len = msg->Utf8Length();
                char buf[len];
                msg->WriteUtf8(buf);

                errmsg->append(":\n");
                errmsg->append(buf, len);
            }
        }
        return scope.Close(result);
    }

    void run(env_t *env) {
        json_result_t result("");
        std::string *errmsg = boost::get<std::string>(&result);

        v8::HandleScope handle_scope;
        v8::Handle<v8::Value> value = eval(errmsg);
        if (!value.IsEmpty()) {
            // JSONify result.
            boost::shared_ptr<scoped_cJSON_t> json = toJSON(value, errmsg);
            if (json) {
                result = json;
            }
        }

        write_message_t msg;
        msg << result;
        guarantee(0 == send_write_message(env->control(), &msg));
    }
};

boost::shared_ptr<scoped_cJSON_t> runner_t::eval(
    const char *src,
    size_t len,
    const std::map<std::string, boost::shared_ptr<scoped_cJSON_t> > &env,
    std::string *errmsg,
    UNUSED req_config_t *config)
{
    json_result_t result;

    {
        run_task_t run(this, eval_task_t(env, std::string(src, len)));
        guarantee(ARCHIVE_SUCCESS == deserialize(this, &result));
    }

    json_visitor_t v(errmsg);
    return boost::apply_visitor(v, result);
}

// // ----- getJSON() -----
// struct get_json_task_t : auto_task_t<get_json_task_t> {
//     get_json_task_t() {}
//     explicit get_json_task_t(id_t id) : id_(id) {}
//     id_t id_;
//     RDB_MAKE_ME_SERIALIZABLE_1(id_);

//     void run(env_t *env) {
//         json_result_t result("");

//         // Get the object.
//         v8::HandleScope handle_scope;
//         v8::Handle<v8::Value> val = env->findValue(id_);
//         rassert(!val.IsEmpty());

//         // JSONify it.
//         std::string errmsg;
//         boost::shared_ptr<scoped_cJSON_t> ptr = toJSON(val, &errmsg);
//         if (ptr) {
//             // Store into result for transport.
//             result = ptr;
//         } else {
//             result = errmsg;
//         }

//         // Send back the result.
//         write_message_t msg;
//         msg << result;
//         guarantee(0 == send_write_message(env->control(), &msg));
//     }
// };

// boost::shared_ptr<scoped_cJSON_t> runner_t::getJSON(
//     const js_handle_t *json_handle,
//     std::string *errmsg,
//     UNUSED req_config_t *config)
// {
//     json_result_t result;
//     {
//         run_task_t(this, get_json_task_t(json_handle->id_));
//         guarantee(ARCHIVE_SUCCESS == deserialize(this, &result));
//     }

//     json_visitor_t v(errmsg);
//     return boost::apply_visitor(v, result);
// }

// TODO (rntz)

} // namespace js
