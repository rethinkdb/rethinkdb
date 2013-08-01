// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef EXTPROC_JS_RUNNER_HPP_
#define EXTPROC_JS_RUNNER_HPP_

#include <string>
#include <vector>
#include <set>

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/variant.hpp>

#include "containers/scoped.hpp"
#include "concurrency/wait_any.hpp"
#include "arch/timing.hpp"
#include "http/json.hpp"

// Unique ids used to refer to objects on the JS side.
typedef uint64_t js_id_t;
const js_id_t INVALID_ID = 0;

// JS calls result either in a DATUM return value, a function id (which we can
// use to call the function later), or an error string
typedef boost::variant<boost::shared_ptr<scoped_cJSON_t>, js_id_t, std::string> js_result_t;

class extproc_pool_t;
class js_runner_t;
class js_job_t;
class js_timeout_sentry_t;

class js_worker_exc_t : public std::exception {
public:
    explicit js_worker_exc_t(const std::string& data) : info(data) { }
    ~js_worker_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

// Useful for managing ID lifetimes.
class js_scoped_id_t {
public:
    explicit js_scoped_id_t(js_runner_t *_parent, js_id_t _id = INVALID_ID);
    ~js_scoped_id_t();

    bool empty() const;
    js_id_t get() const;

    js_id_t release();
    void reset(js_id_t _id = INVALID_ID);

private:
    js_runner_t *parent;
    js_id_t id;

    DISABLE_COPYING(js_scoped_id_t);
};

// A handle to a running "javascript evaluator" job.
class js_runner_t {
public:
    js_runner_t();
    ~js_runner_t();

    bool connected() const;

    // Used for worker configuration
    struct req_config_t {
        uint64_t timeout_ms;
    };

    void begin(extproc_pool_t *pool,
               signal_t *interruptor);

    // Evalute JS source string to either a value or a function ID to call later
    js_result_t eval(const std::string &source,
                     const req_config_t &config);

    // Calls a previously compiled function.
    js_result_t call(js_id_t id,
                     const std::vector<boost::shared_ptr<scoped_cJSON_t> > &args,
                     const req_config_t &config);

    // Invalidates an ID, dereferencing the object it refers to in the
    // javascript evaluator process.
    void release(js_id_t id);

private:
    void note_id(js_id_t id);

    class job_data_t;

    scoped_ptr_t<job_data_t> job_data;


    DISABLE_COPYING(js_runner_t);
};

#endif /* EXTPROC_JS_RUNNER_HPP_ */
