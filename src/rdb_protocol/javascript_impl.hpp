#ifndef RDB_PROTOCOL_JAVASCRIPT_IMPL_HPP_
#define RDB_PROTOCOL_JAVASCRIPT_IMPL_HPP_

#include "rdb_protocol/javascript.hpp"

#include <v8.h>

#include <map>
#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "containers/archive/boost_types.hpp"
#include "rpc/serialize_macros.hpp"

namespace js {

// Returns an empty pointer on error.
boost::shared_ptr<scoped_cJSON_t> toJSON(const v8::Handle<v8::Value> value, std::string *errmsg);

// Should never error.
v8::Handle<v8::Value> fromJSON(const cJSON &json);

// Returns an empty handle and sets `*errmsg` on error.
v8::Handle<v8::Value> eval(const std::string &src, std::string *errmsg);

// Worker-side JS evaluation environment.
class env_t {
  public:
    explicit env_t(extproc::job_t::control_t *control);
    ~env_t();

    // Runs a loop accepting and evaluating task_t's (see below).
    void run();

    id_t rememberValue(v8::Handle<v8::Value> value);
    id_t rememberTemplate(v8::Handle<v8::ObjectTemplate> templ);

    v8::Handle<v8::Value> findValue(id_t id);
    v8::Handle<v8::ObjectTemplate> findTemplate(id_t id);

    void forget(id_t id);

  private:
    id_t new_id();

  public:
    extproc::job_t::control_t *control_;
    bool should_quit_;

  private:
    id_t next_id_;
    std::map<id_t, v8::Persistent<v8::Value> > values_;
    std::map<id_t, v8::Persistent<v8::ObjectTemplate> > templates_;
};

// Puts us into a fresh context.
struct context_t {
    context_t(UNUSED env_t *env) : cx(v8::Context::New()), scope(cx) {}
    ~context_t() { cx.Dispose(); }
    v8::Persistent<v8::Context> cx;
    v8::Context::Scope scope;
};

// Results we get back from tasks, generally "success or error" variants
typedef boost::variant<id_t, std::string> id_result_t;
typedef boost::variant<boost::shared_ptr<scoped_cJSON_t>, std::string> json_result_t;

// Tasks: "mini-jobs" that we run within an env_t
class task_t :
    public extproc::job_t
{
  public:
    virtual void run(env_t *env) = 0;

    void run_job(control_t *control, void *extra) {
        env_t *env = static_cast<env_t *>(extra);
        rassert(control == env->control_);
        context_t cx(env);
        run(env);
    }
};

template <class instance_t>
struct auto_task_t : extproc::auto_job_t<instance_t, task_t> {};

} // namespace js

#endif // RDB_PROTOCOL_JAVASCRIPT_IMPL_HPP_
