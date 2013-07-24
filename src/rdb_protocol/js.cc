// Copyright 2010-2013 RethinkDB, all rights reserved.
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/scoped.hpp"
#include "extproc/job.hpp"
#include "extproc/pool.hpp"
#include "rdb_protocol/jsimpl.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "utils.hpp"

namespace js {

// The actual job that runs all this stuff.
class runner_job_t : public extproc::auto_job_t<runner_job_t> {
public:
    runner_job_t() {}
    virtual void run_job(extproc::job_control_t *control, UNUSED void *extra) {
        // The reason we have env_t is to use it here.
        env_t(control).run();
    }

    RDB_MAKE_ME_SERIALIZABLE_0();
};


const id_t MIN_ID = 1;
// TODO: This is not the max id.  MAX_ID - 1 is the max id.
const id_t MAX_ID = UINT32_MAX;

static void append_caught_error(std::string *errmsg, const v8::TryCatch &try_catch) {
    if (!try_catch.HasCaught()) return;

    v8::String::Utf8Value exception(try_catch.Exception());
    const char *message = *exception;
    guarantee(message);
    errmsg->append(message, strlen(message));
}

scoped_id_t::~scoped_id_t() {
    if (!empty()) reset();
}

void scoped_id_t::reset(id_t id) {
    guarantee(id_ != id);
    if (!empty()) {
        parent_->release_id(id_);
    }
    id_ = id;
}


runner_t::req_config_t::req_config_t()
    : timeout_ms(0)
{}

env_t::env_t(extproc::job_control_t *control)
    : control_(control),
      should_quit_(false),
      next_id_(MIN_ID)
{}

env_t::~env_t() {
    // Clean up handles.
    for (std::map<id_t, v8::Persistent<v8::Value> *>::iterator it = values_.begin();
         it != values_.end();
         ++it)
        it->second->Dispose();
}

void env_t::run() {
    while (!should_quit_) {
        int res = extproc::job_t::accept_job(control_, this);
        guarantee(-1 != res);
    }
}

void env_t::shutdown() { should_quit_ = true; }

id_t env_t::rememberValue(v8::Handle<v8::Value> value) {
    id_t id = new_id();

    // Save this value in a persistent handle so it isn't deallocated when
    // its scope is destructed.
    v8::Persistent<v8::Value> persistent_handle;
    persistent_handle.Reset(v8::Isolate::GetCurrent(), value);

    values_.insert(std::make_pair(id, &persistent_handle));
    return id;
}

v8::Handle<v8::Value> env_t::findValue(id_t id) {
    std::map<id_t, v8::Persistent<v8::Value> *>::iterator it = values_.find(id);
    guarantee(it != values_.end());

    // Construct local handle from persistent handle
    auto local_handle = v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), *(it->second));

    return local_handle;
}

id_t env_t::new_id() {
    guarantee(next_id_ < MAX_ID); // overflow would be bad
    // TODO: What is this?  Is MAX_ID is not maximum id.  Why would
    // you call it MAX_ID if it was not the maximum possible id?  Why
    // are you having code with a side effect on the same line?
    return next_id_++;
}

void env_t::forget(id_t id) {
    guarantee(id < next_id_);
    size_t num_erased = values_.erase(id);
    guarantee(1 == num_erased);
}


runner_t::runner_t()
    : job_handle_(new extproc::job_handle_t), running_task_(false) { }

const runner_t::req_config_t *runner_t::default_req_config() {
    static req_config_t config;
    return &config;
}

// TODO(rntz): should we check that we have no used ids? (ie. no remaining
// handles?)
//
// For now, no, because we don't actually use handles to manage id lifetimes at
// the moment. Instead we tag them onto terms and keep them around for the
// entire query.
runner_t::~runner_t() {
    if (connected()) finish();
}

bool runner_t::connected() { return job_handle_->connected(); }


void runner_t::begin(extproc::pool_t *pool) {
    // TODO(rntz): might eventually want to handle external process failure
    int res = job_handle_->begin(pool, runner_job_t());
    guarantee(0 == res);
}

struct quit_task_t : auto_task_t<quit_task_t> {
    RDB_MAKE_ME_SERIALIZABLE_0();
    void run(env_t *env) { env->shutdown(); }
};

void runner_t::finish() {
    guarantee(connected());
    run_task_t(this, default_req_config(), quit_task_t());
    job_handle_->release();
}

// ----- runner_t::run_task_t -----
runner_t::run_task_t::run_task_t(runner_t *runner, const req_config_t *config,
                                 const task_t &task)
    : runner_(runner)
{
    guarantee(!runner_->running_task_);
    runner_->running_task_ = true;

    if (NULL == config)
        config = runner->default_req_config();
    if (config->timeout_ms)
        timer_.init(new signal_timer_t(config->timeout_ms));

    int res = task.send_over(this);
    guarantee(0 == res);
}

runner_t::run_task_t::~run_task_t() {
    guarantee(runner_->running_task_);
    runner_->running_task_ = false;
}

int64_t runner_t::run_task_t::read(void *p, int64_t n) {
    return runner_->job_handle_->read_interruptible(p, n, timer_.has() ? timer_.get() : NULL);
}

int64_t runner_t::run_task_t::write(const void *p, int64_t n) {
    return runner_->job_handle_->write_interruptible(p, n, timer_.has() ? timer_.get() : NULL);
}


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
    guarantee(connected());
    std::set<id_t>::iterator it = used_ids_.find(id);
    guarantee(it != used_ids_.end());

    run_task_t(this, default_req_config(), release_task_t(id));

    used_ids_.erase(it);
}

// ----- eval() -----
struct eval_task_t : auto_task_t<eval_task_t> {
    eval_task_t() {}
    explicit eval_task_t(const std::string &src) : src_(src) {}

    std::string src_;
    RDB_MAKE_ME_SERIALIZABLE_1(src_);

    void run(env_t *env) {
        js_result_t result("");
        std::string *errmsg = boost::get<std::string>(&result);

        v8::HandleScope handle_scope;

        // TODO(rntz): use an "external resource" to avoid copy?
        v8::Handle<v8::String> src = v8::String::New(src_.data(), src_.size());

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
                    result = env->rememberValue(func);

                } else {
                    guarantee(!result_val.IsEmpty());

                    // JSONify result.
                    boost::shared_ptr<scoped_cJSON_t> json = toJSON(result_val, errmsg);
                    if (json) {
                        result = json;
                    }
                }
            }
        }

        write_message_t msg;
        msg << result;
        int sendres = send_write_message(&env->control()->unix_socket, &msg);
        guarantee(0 == sendres);
    }
};

js_result_t runner_t::eval(
    const std::string &source,
    const req_config_t *config)
{
    js_result_t result;

    {
        run_task_t run(this, config, eval_task_t(source));
        int res = deserialize(&run, &result);
        guarantee(ARCHIVE_SUCCESS == res);
    }

    // We need to "note" any function id generated by this eval
    id_t *any_id = boost::get<id_t>(&result);
    if (any_id) {
        note_id(*any_id);
    }

    return result;
}

struct call_task_t : auto_task_t<call_task_t> {
    call_task_t() {}
    call_task_t(id_t id, const std::vector<boost::shared_ptr<scoped_cJSON_t> > &args)
        : func_id_(id), args_(args)
    { }

    id_t func_id_;
    std::vector<boost::shared_ptr<scoped_cJSON_t> > args_;
    RDB_MAKE_ME_SERIALIZABLE_2(func_id_, args_);

    v8::Handle<v8::Value> eval(v8::Handle<v8::Function> func, std::string *errmsg) {
        v8::TryCatch try_catch;
        v8::HandleScope scope;

        // Construct receiver object.
        v8::Handle<v8::Object> obj = v8::Object::New();
        guarantee(!obj.IsEmpty());

        // Construct arguments.
        size_t nargs = args_.size();

        scoped_array_t<v8::Handle<v8::Value> > handles(nargs);
        for (size_t i = 0; i < nargs; ++i) {
            handles[i] = fromJSON(*args_[i]->get());
            guarantee(!handles[i].IsEmpty());
        }

        // Call function with environment as its receiver.
        v8::Handle<v8::Value> result = func->Call(obj, nargs, handles.data());
        if (result.IsEmpty()) {
            append_caught_error(errmsg, try_catch);
        }
        return scope.Close(result);
    }

    void run(env_t *env) {
        // TODO(rntz): This is very similar to compile_task_t::run(). Refactor?
        js_result_t result("");
        std::string *errmsg = boost::get<std::string>(&result);

        v8::HandleScope handle_scope;
        v8::Handle<v8::Function> func
            = v8::Handle<v8::Function>::Cast(env->findValue(func_id_));
        guarantee(!func.IsEmpty());

        v8::Handle<v8::Value> value = eval(func, errmsg);
        if (!value.IsEmpty()) {

            if (value->IsFunction()) {
                v8::Handle<v8::Function> sub_func
                    = v8::Handle<v8::Function>::Cast(value);
                result = env->rememberValue(sub_func);
            } else {

                // JSONify result.
                boost::shared_ptr<scoped_cJSON_t> json = toJSON(value, errmsg);
                if (json) {
                    result = json;
                }
            }
        }

        write_message_t msg;
        msg << result;
        int sendres = send_write_message(&env->control()->unix_socket, &msg);
        guarantee(0 == sendres);
    }
};

js_result_t runner_t::call(
    id_t func_id,
    const std::vector<boost::shared_ptr<scoped_cJSON_t> > &args,
    const req_config_t *config)
{
    js_result_t result;

    {
        run_task_t run(this, config, call_task_t(func_id, args));
        int res = deserialize(&run, &result);
        guarantee(ARCHIVE_SUCCESS == res);
    }

    return result;
}

} // namespace js
