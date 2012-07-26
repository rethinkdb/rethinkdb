#ifndef RDB_PROTOCOL_JS_HPP_
#define RDB_PROTOCOL_JS_HPP_

#include <map>
#include <set>
#include <string>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "extproc/job.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rpc/serialize_macros.hpp"

namespace js {

class runner_t;
class task_t;

// Unique ids used to refer to objects on the JS side.
typedef uint32_t id_t;

// Handles to objects in the JS job process.
class handle_t {
    friend class runner_t;

  public:
    handle_t() : parent_(NULL) {}
    handle_t(runner_t *parent, id_t id) : parent_(parent), id_(id) {}
    virtual ~handle_t();

    // returns true iff we hold no ref
    bool empty() const { return parent_ == NULL; }

    id_t get() const;

    // releases an id without destroying its referent
    // PRECOND: we hold a ref
    // POSTCOND: we do not, but our previous id still has a referent
    id_t release();

    // destroys our referent
    // PRECOND: we hold a ref
    // POSTCOND: we do not, and our previous id has no referent
    void reset();

  private:
    runner_t *parent_;
    id_t id_;

    DISABLE_COPYING(handle_t);
};

// Subclasses of handle_t are functionally identical, and differ only for C++
// type-checking purposes. Currently we never manipulate handle_ts that are not
// js_handle_ts or a subtype thereof.

// Handle to a JS object.
struct js_handle_t : handle_t {
    js_handle_t() {}
    js_handle_t(runner_t *parent, id_t id) : handle_t(parent, id) {}
};

// Handle to a JS "method": a function and associated receiver object "template"
// (set of properties).
struct method_handle_t : handle_t {
    method_handle_t() {}
    method_handle_t(runner_t *parent, id_t id) : handle_t(parent, id) {}
};


// A handle to a running "javascript evaluator" job.
class runner_t :
    private extproc::job_handle_t
{
    friend class handle_t;
    friend class run_task_t;

  public:
    runner_t();
    ~runner_t();

    // For now we crash on errors, but eventually we may need to deal with job
    // failure more cleanly; we will probably use exceptions.
    struct job_fail_exc_t {
        std::string message;
    };

    bool begun() { return extproc::job_handle_t::connected(); }

    void begin(extproc::pool_t *pool);
    void finish();
    void interrupt();

    // Generic per-request options. A pointer to one of these is passed to all
    // requests. If NULL, the default configuration is used.
    struct req_config_t {
        req_config_t();
        long timeout;           // FIXME: wrong type.
    };

    // Returns false on error.
    MUST_USE bool compile(
        method_handle_t *out,
        // Argument names
        const std::vector<std::string> &args,
        // Source for the body of the function, _not_ including opening
        // "function(...) {" and closing "}".
        const std::string &source,
        std::string *errmsg,
        req_config_t *config = NULL);

    // Calls a previously compiled function.
    // TODO: receiver object.
    boost::shared_ptr<scoped_cJSON_t> call(
        const method_handle_t *handle,
        const std::vector<boost::shared_ptr<scoped_cJSON_t> > &args,
        std::string *errmsg,
        req_config_t *config = NULL);

    // TODO (rntz): a way to send streams over to javascript.
    // TODO (rntz): a way to get streams back from javascript.
    // TODO (rntz): map/reduce jobs & co.

  private:
    // The actual job that runs all this stuff.
    class job_t :
        public extproc::auto_job_t<job_t>
    {
      public:
        job_t() {}
        virtual void run_job(control_t *control, void *extra);
        RDB_MAKE_ME_SERIALIZABLE_0();
    };

    class run_task_t {
      public:
        // Starts running the given task. We can only run one task at a time.
        run_task_t(runner_t *runner, const task_t &task);
        // Signals that we are done running this task.
        ~run_task_t();

      private:
        runner_t *runner_;
        DISABLE_COPYING(run_task_t);
    };

  private:
    void create_handle(handle_t *handle, id_t id);

#ifndef NDEBUG
    void on_new_id(id_t id);
#endif
    void release_id(id_t id);

    // The default req_config_t for this runner_t. May be modified to change
    // defaults.
  public:
    req_config_t default_req_config;

  private:
#ifndef NDEBUG
    bool running_task_;
    std::set<id_t> used_ids_;
#endif
};

} // namespace js

#endif // RDB_PROTOCOL_JS_HPP_
