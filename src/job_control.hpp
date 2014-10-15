// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef JOB_CONTROL_HPP_
#define JOB_CONTROL_HPP_

#include <string>
#include <vector>

#include "concurrency/cond_var.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"
#include "time.hpp"

class job_desc_t
{
public:
    job_desc_t(std::string const & description, uuid_u const & uuid, microtime_t start_time);

    std::string description;
    uuid_u uuid;
    microtime_t start_time;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(job_desc_t);

class job_sentry_t
{
public:
    job_sentry_t(std::string const & description);
    ~job_sentry_t();

    job_desc_t const & get_job_desc();
    signal_t *get_interruptor_signal();

private:
    job_desc_t job_description;
    cond_t interrupt_cond;

    DISABLE_COPYING(job_sentry_t);
};

std::vector<job_desc_t> get_job_descriptions();

#endif // JOB_CONTROL_HPP_
