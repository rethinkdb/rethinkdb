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

// A standard "id or error" result.
typedef boost::variant<id_t, std::string> id_result_t;

// "Mini-jobs" that we run within an env_t.
class task_t :
    public extproc::job_t
{
  public:
    virtual void run(env_t *env) = 0;

    void run_job(control_t *control, void *extra) {
        env_t *env = static_cast<env_t *>(extra);
        rassert(control == env->control_);
        run(env);
    }
};

template <class instance_t>
struct auto_task_t : extproc::auto_job_t<instance_t, task_t> {};

class eval_task_t :
        public auto_task_t<eval_task_t>
{
  public:
    eval_task_t() {}
    explicit eval_task_t(const std::string &src) : src_(src) {}

    void run(env_t *env);

    RDB_MAKE_ME_SERIALIZABLE_1(src_);

  private:
    void eval(env_t *env, v8::Handle<v8::Context> cx, id_result_t *result);

  private:
    std::string src_;
};

} // namespace js

#endif // RDB_PROTOCOL_JAVASCRIPT_IMPL_HPP_
