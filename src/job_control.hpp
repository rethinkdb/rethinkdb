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

enum class job_type_t { BACKFILL, INDEX_CONSTRUCTION, QUERY, GARBAGE_COLLECTION };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(job_type_t,
                                      int8_t,
                                      job_type_t::BACKFILL,
                                      job_type_t::GARBAGE_COLLECTION);

inline std::string job_type_t_to_string(job_type_t job_type) {
    std::string string;

    switch (job_type) {
    case job_type_t::BACKFILL:
        string = "backfill";
        break;
    case job_type_t::INDEX_CONSTRUCTION:
        string = "index_construction";
        break;
    case job_type_t::QUERY:
        string = "query";
        break;
    case job_type_t::GARBAGE_COLLECTION:
        string = "garbage_collection";
        break;
    default:
        unreachable();
    }

    return string;
}

/* The `job_sentry_t` stores a `job_entry_t` in a thead-local map, recording the
   starting time of a job. When responding to a jobs query we want to return the
   duration, thus it's converted to a `job_wire_entry_t`. */

class job_entry_t {
public:
    job_entry_t();
    job_entry_t(job_type_t type, uuid_u const & id, microtime_t start_time);

    job_type_t type;
    uuid_u id;
    microtime_t start_time;
};

class job_wire_entry_t {
public:
    job_wire_entry_t();
    job_wire_entry_t(const job_entry_t &entry);

    job_type_t type;
    uuid_u id;
    double duration;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(job_wire_entry_t);

class job_sentry_t
{
public:
    job_sentry_t(job_type_t type);
    ~job_sentry_t();

    job_entry_t const & get_job_entry();
    signal_t *get_interruptor_signal();

private:
    job_entry_t job_entry;
    cond_t interruptor;

    DISABLE_COPYING(job_sentry_t);
};

std::vector<job_wire_entry_t> get_job_wire_entries();

#endif // JOB_CONTROL_HPP_
