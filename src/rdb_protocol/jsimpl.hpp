// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_JSIMPL_HPP_
#define RDB_PROTOCOL_JSIMPL_HPP_

#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 406)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include <v8.h>
#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 406)
#pragma GCC diagnostic pop
#endif

#include <map>
#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>

#include "containers/archive/boost_types.hpp"
#include "extproc/job.hpp"
#include "http/json.hpp"
#include "rdb_protocol/js.hpp"
#include "rpc/serialize_macros.hpp"

namespace extproc {
class job_t;
class job_control_t;
};

namespace js {

// Returns an empty pointer on error.
boost::shared_ptr<scoped_cJSON_t> toJSON(const v8::Handle<v8::Value> &value,
                                         std::string *errmsg);

// Should never error.
v8::Handle<v8::Value> fromJSON(const cJSON &json);


// Worker-side JS evaluation environment.
class env_t {
    friend class runner_job_t;

  private:                      // Interface used by runner_job_t().
    explicit env_t(extproc::job_control_t *control);
    ~env_t();

    // Runs a loop accepting and evaluating task_t's (see below).
    // Used by runner_t::job_t::run_job();
    void run();

  public:                       // Interface exposed to JS tasks.
    extproc::job_control_t *control() { return control_; }

    id_t rememberValue(const v8::Handle<v8::Value> &value);

    const boost::shared_ptr<v8::Persistent<v8::Value> >findValue(id_t id);

    void forget(id_t id);

    // Arranges for us to quit (stop accepting tasks).
    void shutdown();

  private:                      // Internal helper functions.
    // Generates a fresh, unused identifier. Used by remember*().
    id_t new_id();

  private:                      // Fields
    extproc::job_control_t *control_;
    bool should_quit_;
    id_t next_id_;
    std::map<id_t, boost::shared_ptr<v8::Persistent<v8::Value> > > values_;
};


// Puts us into a fresh v8 context.
// By default each task gets its own context.
struct context_t {
#ifdef V8_PRE_3_19
    explicit context_t(UNUSED env_t *env) :
        cx(v8::Context::New()),
        scope(cx) {}

    v8::Persistent<v8::Context> cx;
    v8::Context::Scope scope;
#else
    explicit context_t(UNUSED env_t *env) :
        cx(v8::Context::New(v8::Isolate::GetCurrent())),
        scope(cx) {}

    v8::HandleScope local_scope;
    v8::Local<v8::Context> cx;
    v8::Context::Scope scope;
#endif
};

// Tasks: jobs we run on the JS worker, within an env_t
class task_t :
    private extproc::job_t
{
    friend class runner_t;

  public:
    virtual void run(env_t *env) = 0;

    void run_job(extproc::job_control_t *control, void *extra) {
        env_t *env = static_cast<env_t *>(extra);
        guarantee(control == env->control());
        context_t cx(env);
        run(env);
    }
};

template <class instance_t>
struct auto_task_t : extproc::auto_job_t<instance_t, task_t> {};

} // namespace js

#endif // RDB_PROTOCOL_JSIMPL_HPP_
