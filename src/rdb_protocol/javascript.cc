#define __STDC_LIMIT_MACROS     // hack. :(
#include <stdint.h>
#define MAX_ID UINT32_MAX

#include "rdb_protocol/javascript_impl.hpp"

#include "utils.hpp"

namespace js {

eval_t::req_config_t::req_config_t()
    : timeout(0)
{}


// ---------- env_t ----------
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

// ---------- eval_task_t ----------
void eval_task_t::run(env_t *env) {
    id_result_t res("unknown");

    // TODO (rntz): should we have one context per env?
    v8::Persistent<v8::Context> cx = v8::Context::New();
    eval(env, cx, &res);
    cx.Dispose();

    // Send back result.
    write_message_t msg;
    msg << res;
    guarantee(0 == send_write_message(env->control_, &msg));
}

void eval_task_t::eval(env_t *env, v8::Handle<v8::Context> cx, id_result_t *result) {
    v8::Context::Scope cx_scope(cx);
    v8::HandleScope handle_scope;
    v8::Handle<v8::String> src = v8::String::New(src_.c_str());

    v8::TryCatch try_catch;

    v8::Handle<v8::Script> script = v8::Script::Compile(src);
    if (script.IsEmpty()) {
        *result = "script compilation failed";
        return;
    }

    // TODO(rntz): better error reporting (backtraces?)
    v8::Handle<v8::Value> value = script->Run();
    if (value.IsEmpty()) {
        *result = "script execution failed";
        return;
    }

    *result = env->rememberValue(value);
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


// ---------- eval_t ----------
eval_t::eval_t()
    : DEBUG_ONLY(running_task_(false))
{}

void eval_t::job_t::run_job(control_t *control, UNUSED void *extra) {
    // The reason we have env_t is to use it here.
    env_t(control).run();
}

#ifndef NDEBUG
void eval_t::on_new_id(id_t id) {
    rassert(connected());
    rassert(!used_ids_.count(id));
    used_ids_.insert(id);
}
#endif

void eval_t::begin_task(const task_t &task) {
    rassert(!running_task_);
    DEBUG_ONLY_CODE(running_task_ = true);
    guarantee(0 == task.send_over(this));
}

void eval_t::end_task() {
    rassert(running_task_);
    DEBUG_ONLY_CODE(running_task_ = false);
}

void eval_t::init_handle(handle_t *handle, id_t id) {
    rassert(handle->empty());
    handle->parent_ = this;
    handle->id_ = id;
    on_new_id(id);
}

// ---------- tasks ----------
struct release_task_t : auto_task_t<release_task_t> {
    release_task_t() {}
    explicit release_task_t(id_t id) : id_(id) {}
    id_t id_;
    RDB_MAKE_ME_SERIALIZABLE_1(id_);
    void run(env_t *env) {
        env->forget(id_);
    }
};

void eval_t::release_id(id_t id) {
    rassert(connected());
    rassert(used_ids_.count(id));

    begin_task(release_task_t(id));
    end_task();

    DEBUG_ONLY_CODE(used_ids_.erase(id));
}

struct id_visitor_t {
    typedef bool result_type;
    id_visitor_t(std::string *errmsg) : errmsg_(errmsg) {}
    id_t id_;
    std::string *errmsg_;
    bool operator()(const id_t &id) { id_ = id; return true; }
    bool operator()(const std::string &msg) { *errmsg_ = msg; return false; }
};

bool eval_t::eval(
    js_handle_t *out,
    const char *src,
    size_t len,
    std::string *errmsg,
    UNUSED req_config_t *config)
{
    id_result_t result;

    begin_task(eval_task_t(std::string(src, len)));
    guarantee(ARCHIVE_SUCCESS == deserialize(this, &result));
    end_task();

    id_visitor_t v(errmsg);
    if (boost::apply_visitor(v, result)) {
        init_handle(out, v.id_);
        return true;
    } else {
        return false;
    }
}

// TODO (rntz)

} // namespace js
