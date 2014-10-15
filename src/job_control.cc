// Copyright 2010-2014 RethinkDB, all rights reserved.

#include "job_control.hpp"

#include <map>

#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"
#include "thread_local.hpp"

typedef std::map<uuid_u, job_sentry_t *> job_map_t;
TLS(job_map_t, job_map);

job_desc_t::job_desc_t(std::string const & _description,
                       uuid_u const & _uuid,
                       microtime_t _start_time)
    : description(_description),
      uuid(_uuid),
      start_time(_start_time)
{
}

job_sentry_t::job_sentry_t(std::string const & description)
    : job_description(description, generate_uuid(), current_microtime())
{
    TLS_get_ref_job_map().insert(std::make_pair(job_description.uuid, this));
}

job_sentry_t::~job_sentry_t()
{
    TLS_get_ref_job_map().erase(job_description.uuid);
}

job_desc_t const & job_sentry_t::get_job_desc() {
    return job_description;
}

signal_t *job_sentry_t::get_interruptor_signal()
{
    return &interrupt_cond;
}

std::vector<job_desc_t> get_job_descriptions()
{
    std::vector<job_desc_t> job_descs;
    pmap(get_num_threads(), [&](int32_t threadnum) {
        std::vector<job_desc_t> coro_job_descs;
        {
            on_thread_t thread((threadnum_t(threadnum)));

            for (auto const & job : TLS_get_ref_job_map()) {
                coro_job_descs.push_back(job.second->get_job_desc());
            }
        }

        job_descs.insert(job_descs.end(),
                         coro_job_descs.begin(),
                         coro_job_descs.end());
    });

    return job_descs;
}
