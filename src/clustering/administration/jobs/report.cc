// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/report.hpp"

job_report_t::job_report_t() {
}

job_report_t::job_report_t(std::string const &_type, uuid_u const &_id, double _duration)
    : type(_type),
      id(_id),
      duration(_duration) {
}

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(job_report_t, type, id, duration);

query_job_t::query_job_t(microtime_t _start_time)
    : start_time(_start_time) {
}

sindex_job_t::sindex_job_t(microtime_t _start_time)
    : start_time(_start_time) {
};
