// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef EXTPROC_WGET_RUNNER_HPP_
#define EXTPROC_WGET_RUNNER_HPP_

#include <string>
#include <vector>
#include <set>

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/variant.hpp>

#include "containers/scoped.hpp"
#include "containers/counted.hpp"
#include "rdb_protocol/datum.hpp"
#include "concurrency/wait_any.hpp"
#include "arch/timing.hpp"
#include "http/json.hpp"

// wget calls result either in a DATUM return value, a function id (which we can
// use to call the function later), or an error string
typedef boost::variant<counted_t<const ql::datum_t>, std::string> wget_result_t;

class extproc_pool_t;
class wget_runner_t;
class wgetjs_job_t;

class wget_worker_exc_t : public std::exception {
public:
    explicit wget_worker_exc_t(const std::string& data) : info(data) { }
    ~wget_worker_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

// A handle to a running "javascript evaluator" job.
class wget_runner_t : public home_thread_mixin_t {
public:
    wget_runner_t(extproc_pool_t *_pool);

    wget_result_t wget(const std::string &url,
                       const std::vector<std::string> &headers,
                       size_t rate_limit,
                       uint64_t timeout_ms,
                       signal_t *interruptor);

private:
    extproc_pool_t *pool;

    DISABLE_COPYING(wget_runner_t);
};

#endif /* EXTPROC_WGET_RUNNER_HPP_ */
