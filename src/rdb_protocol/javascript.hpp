#ifndef RDB_PROTOCOL_JAVASCRIPT_HPP_
#define RDB_PROTOCOL_JAVASCRIPT_HPP_

#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "extproc/job.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rpc/serialize_macros.hpp"

namespace js {

class eval_t;
class task_t;                   // in javascript_impl.hpp

// Unique ids used to refer to objects on the JS side.
typedef uint32_t id_t;

// Handles to objects in the JS job process.
class handle_t {
    friend class eval_t;

  public:
    handle_t() : parent_(NULL) {}
    virtual ~handle_t();

    // returns true iff we hold no ref
    bool empty() const { return parent_ == NULL; }
    void release();         // precond: we hold a ref; postcond: we do not

  private:
    eval_t *parent_;
    id_t id_;

    DISABLE_COPYING(handle_t);
};

// Subclasses of handle_t are functionally identical, and differ only for C++
// type-checking purposes.

// Handlet to a JS object.
struct js_handle_t : handle_t { js_handle_t() {} };

// Handle to a JS function.
struct function_handle_t : js_handle_t { function_handle_t() {} };

// Handle to a "template" that can be used to manufacture objects.
struct template_handle_t : handle_t { template_handle_t() {} };

// A handle to a running "javascript evaluator" job.
class eval_t :
    private extproc::job_handle_t
{
  public:
    eval_t();

    void begin(extproc::pool_t *pool);
    void finish();
    void interrupt();

    // For now we crash on errors, but eventually we may need to deal with job
    // failure more cleanly; we will probably use exceptions.
    struct job_fail_exc_t {
        std::string message;
    };

    // Many functions below are "requests" which return a bool and take a
    // `std::string *errmsg` parameter.
    //
    // - On success, they return true.
    // - On failure, they return false, putting an error message in `*errmsg`.
    //
    // This notion of failure is distinct from job failure; if a request below
    // fails, *only* that request failed; the eval_t remains usable.

    // Generic per-request options. A pointer to one of these is passed to all
    // requests. If NULL, the default configuration is used.
    struct req_config_t {
        req_config_t();
        long timeout;           // FIXME: wrong type.
    };

    // Evaluates javascript, storing a handle to it in `*out`.
    // May fail if the javascript does not compile or run.
    //
    // TODO(rntz): a way to return the result as JSON immediately?
    MUST_USE bool eval(
        js_handle_t *out,
        const char *src,
        size_t len,
        std::string *errmsg,
        req_config_t *config = NULL);

    // Compiles a function and hands back a reference to it.
    // May fail for the same reasons as `eval`.
    MUST_USE bool compile(
        function_handle_t *out,
        const char *src,
        size_t len,
        std::string *errmsg,
        req_config_t *config = NULL);

    // Sends over a JSON object. Can't fail (except due to job failure).
    void fromJSON(
        js_handle_t *out,
        const boost::shared_ptr<scoped_cJSON_t> json,
        req_config_t *config = NULL);

    // Retrieves an object as JSON from the other side.
    // Fails if the object can't be represented as JSON.
    MUST_USE bool getJSON(
        boost::shared_ptr<scoped_cJSON_t> *out,
        const js_handle_t *json_handle,
        std::string *errmsg,
        req_config_t *config = NULL);

    // Creates a "template" from a given set of property names. A template can
    // be used to quickly manufacture efficient "instances", which are objects
    // with the given set of properties.
    void makeTemplate(
        template_handle_t *out,
        std::vector<std::string> prop_names,
        req_config_t *config = NULL);

    // Creates an instance of a template.
    void makeInstance(
        js_handle_t *out,
        const template_handle_t *templ,
        std::vector<const handle_t *> prop_vals,
        req_config_t *config = NULL);

    // Calls a function.
    MUST_USE bool call(
        // `receiver` is what becomes "this" in the function invocation.
        // it may be NULL to use a fresh object.
        const js_handle_t *receiver,
        const function_handle_t *func,
        int argc,
        const js_handle_t **argv,
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

  private:
    friend class handle_t;

#ifndef NDEBUG
    void on_new_id(id_t id);
#endif
    void release_id(id_t id);

    void init_handle(handle_t *handle, id_t id);

    void begin_task(const task_t &task);
    void end_task();

    // The default req_config_t for this eval_t. May be modified to change
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

#endif // RDB_PROTOCOL_JAVASCRIPT_HPP_
