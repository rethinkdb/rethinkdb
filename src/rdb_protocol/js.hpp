#ifndef RDB_PROTOCOL_JS_HPP_
#define RDB_PROTOCOL_JS_HPP_

#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "arch/timing.hpp"      // signal_timer_t
#include "containers/archive/archive.hpp"
#include "containers/scoped.hpp"
#include "extproc/job.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rpc/serialize_macros.hpp"

namespace js {

class runner_t;
class task_t;

// Unique ids used to refer to objects on the JS side.
typedef uint32_t id_t;
const id_t INVALID_ID = 0;

// Useful for managing ID lifetimes.
class scoped_id_t {
    friend class runner_t;

  public:
    explicit scoped_id_t(runner_t *parent, id_t id = INVALID_ID) : parent_(parent), id_(id) {}
    ~scoped_id_t();

    bool empty() const { return id_ == INVALID_ID; }

    id_t get() const { return id_; }

    id_t release() {
        id_t id = id_;
        id_ = INVALID_ID;
        return id;
    }

    void reset(id_t id = INVALID_ID);

  private:
    runner_t *parent_;
    id_t id_;

    DISABLE_COPYING(scoped_id_t);
};


// A handle to a running "javascript evaluator" job.
class runner_t :
    private extproc::job_handle_t
{
    friend class run_task_t;

  public:
    runner_t();
    ~runner_t();

    // For now we crash on errors, but eventually we may need to deal with job
    // failure more cleanly; we will probably use exceptions.
    struct job_fail_exc_t {
        std::string message;
    };

    bool connected() { return extproc::job_handle_t::connected(); }

    void begin(extproc::pool_t *pool);
    void finish();
    void interrupt();

    // Invalidates an ID, dereferencing the object it refers to in the
    // javascript evaluator process.
    void release_id(id_t id);

    // Generic per-request options. A pointer to one of these is passed to all
    // requests. If NULL, the default configuration is used.
    //
    // Methods taking a `const req_config_t *config` may time-out and raise
    // interrupted_exc_t if timeout_ms != 0.
    struct req_config_t {
        req_config_t();
        // Time-out in milliseconds. 0 indicates "no timeout". Must be >= 0.
        // Default: 0
        int64_t timeout_ms;
    };

    static const req_config_t *default_req_config();

    // Returns INVALID_ID on error.
    // Returned id may only be used in `call`.
    MUST_USE id_t compile(
        // Argument names
        const std::vector<std::string> &args,
        // Source for the body of the function, _not_ including opening
        // "function(...) {" and closing "}".
        const std::string &source,
        std::string *errmsg,
        const req_config_t *config = NULL);

    // Calls a previously compiled function.
    // TODO: receiver object.
    boost::shared_ptr<scoped_cJSON_t> call(
        id_t func_id,
        const std::vector<boost::shared_ptr<scoped_cJSON_t> > &args,
        std::string *errmsg,
        const req_config_t *config = NULL);

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

    class run_task_t : public read_stream_t, public write_stream_t {
      public:
        // Starts running the given task. We can only run one task at a time.
        run_task_t(runner_t *runner, const req_config_t *config, const task_t &task);
        // Signals that we are done running this task.
        ~run_task_t();

        virtual MUST_USE int64_t read(void *p, int64_t n);
        virtual int64_t write(const void *p, int64_t n);

      private:
        runner_t *runner_;
        scoped_ptr_t<signal_timer_t> timer_;
        DISABLE_COPYING(run_task_t);
    };

  private:
    id_t new_id(id_t id) {
        rassert(connected());
#ifndef NDEBUG
        rassert(!used_ids_.count(id));
        if (id != INVALID_ID) {
            used_ids_.insert(id);
        }
#endif
        return id;
    }

  private:
#ifndef NDEBUG
    bool running_task_;
    std::set<id_t> used_ids_;
#endif
};

} // namespace js

#endif // RDB_PROTOCOL_JS_HPP_
