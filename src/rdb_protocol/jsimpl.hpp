#ifndef RDB_PROTOCOL_JSIMPL_HPP_
#define RDB_PROTOCOL_JSIMPL_HPP_

#include <v8.h>

#include <map>
#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>

#include "containers/archive/boost_types.hpp"
#include "http/json.hpp"
#include "rdb_protocol/js.hpp"
#include "rpc/serialize_macros.hpp"

namespace js {

// Returns an empty pointer on error.
boost::shared_ptr<scoped_cJSON_t> toJSON(const v8::Handle<v8::Value> value, std::string *errmsg);

// Should never error.
v8::Handle<v8::Value> fromJSON(const cJSON &json);


// Worker-side JS evaluation environment.
class env_t {
    friend class runner_t;

  private:                      // Interface used by runner_t::job_t().
    explicit env_t(extproc::job_t::control_t *control);
    ~env_t();

    // Runs a loop accepting and evaluating task_t's (see below).
    // Used by runner_t::job_t::run_job();
    void run();

  public:                       // Interface exposed to JS tasks.
    extproc::job_t::control_t *control() { return control_; }

    id_t rememberValue(v8::Handle<v8::Value> value);

    v8::Handle<v8::Value> findValue(id_t id);

    void forget(id_t id);

    // Arranges for us to quit (stop accepting tasks).
    void shutdown();

  private:                      // Internal helper functions.
    // Generates a fresh, unused identifier. Used by remember*().
    id_t new_id();

  private:                      // Fields
    extproc::job_t::control_t *control_;
    bool should_quit_;
    id_t next_id_;
    std::map<id_t, v8::Persistent<v8::Value> > values_;
};


// Puts us into a fresh v8 context.
// By default each task gets its own context.
struct context_t {
    explicit context_t(UNUSED env_t *env) : cx(v8::Context::New()), scope(cx) {}
    ~context_t() { cx.Dispose(); }
    v8::Persistent<v8::Context> cx;
    v8::Context::Scope scope;
};

// Tasks: jobs we run on the JS worker, within an env_t
class task_t :
    private extproc::job_t
{
    friend class runner_t;

  public:
    virtual void run(env_t *env) = 0;

    void run_job(control_t *control, void *extra) {
        env_t *env = static_cast<env_t *>(extra);
        guarantee_reviewed(control == env->control());
        context_t cx(env);
        run(env);
    }
};

template <class instance_t>
struct auto_task_t : extproc::auto_job_t<instance_t, task_t> {};

// Results we get back from tasks, generally "success or error" variants
typedef boost::variant<id_t, std::string> id_result_t;
typedef boost::variant<boost::shared_ptr<scoped_cJSON_t>, std::string> json_result_t;

// Visitors to extract values from results.
struct id_visitor_t {
    typedef id_t result_type;
    explicit id_visitor_t(std::string *errmsg) : errmsg_(errmsg) {}
    std::string *errmsg_;
    id_t operator()(const id_t &id) {
        guarantee_reviewed(id != INVALID_ID);
        return id;
    }
    id_t operator()(const std::string &msg) {
        *errmsg_ = msg;
        return INVALID_ID;
    }
};

struct json_visitor_t {
    typedef boost::shared_ptr<scoped_cJSON_t> result_type;
    explicit json_visitor_t(std::string *errmsg) : errmsg_(errmsg) {}
    std::string *errmsg_;
    result_type operator()(const result_type &r) { return r; }
    result_type operator()(const std::string &msg) {
        *errmsg_ = msg;
        return result_type();
    }
};

} // namespace js

#endif // RDB_PROTOCOL_JSIMPL_HPP_
