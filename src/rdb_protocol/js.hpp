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
    virtual ~handle_t();

    // returns true iff we hold no ref
    bool empty() const { return parent_ == NULL; }
    void release();         // precond: we hold a ref; postcond: we do not

  private:
    runner_t *parent_;
    id_t id_;

    DISABLE_COPYING(handle_t);
};

// Subclasses of handle_t are functionally identical, and differ only for C++
// type-checking purposes.

// Handle to a JS object.
struct js_handle_t : handle_t { js_handle_t() {} };

// Handle to a JS function.
struct function_handle_t : js_handle_t { function_handle_t() {} };

// Handle to a "template" that can be used to manufacture objects.
struct template_handle_t : handle_t { template_handle_t() {} };


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

    void begin(extproc::pool_t *pool);
    void finish();
    void interrupt();

    // FIXME: this is useless.
    // Generic per-request options. A pointer to one of these is passed to all
    // requests. If NULL, the default configuration is used.
    struct req_config_t {
        req_config_t();
        long timeout;           // FIXME: wrong type.
    };

    // FIXME these are legacy methods, remove them.
    MUST_USE boost::shared_ptr<scoped_cJSON_t> eval(
        const char *src,
        size_t len,
        const std::map<std::string, boost::shared_ptr<scoped_cJSON_t> > &env,
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
