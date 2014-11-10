// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_
#define CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_

#include <string>

#include "containers/archive/stl_types.hpp"
#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"
#include "time.hpp"

class job_report_t {
public:
    job_report_t();
    job_report_t(std::string const &type, uuid_u const &id, double duration);

    std::string type;
    uuid_u id;
    double duration;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(job_report_t);

class query_job_t {
public:
    query_job_t(microtime_t start_time);

    microtime_t start_time;
};

class sindex_job_t {
public:
    sindex_job_t(microtime_t start_time);

    microtime_t start_time;
};

#endif /* CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_ */
