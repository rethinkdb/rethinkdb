// Copyright 2010-2014 RethinkDB, all rights reserved.

#include "job_control.hpp"

#include <map>

#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"
#include "thread_local.hpp"

typedef std::map<uuid_u, job_sentry_t *> job_map_t;
TLS_with_init(job_map_t *, job_map, nullptr);

job_entry_t::job_entry_t() {
}

job_entry_t::job_entry_t(job_type_t _type,
                         uuid_u const & _id,
                         microtime_t _start_time)
    : type(_type),
      id(_id),
      start_time(_start_time)
{
}

job_wire_entry_t::job_wire_entry_t() {
}

job_wire_entry_t::job_wire_entry_t(const job_entry_t &entry)
    : type(entry.type),
      id(entry.id),
      duration((current_microtime() - entry.start_time) / 1000000.0)
{
}
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(job_wire_entry_t, type, id, duration);

job_sentry_t::job_sentry_t(job_type_t type)
    : job_entry(type, generate_uuid(), current_microtime())
{
    job_map_t *job_map = TLS_get_job_map();
    if (job_map == nullptr) {
        job_map = new job_map_t{};
    }

    job_map->insert(std::make_pair(job_entry.id, this));

    TLS_set_job_map(job_map);
}

job_sentry_t::~job_sentry_t()
{
    job_map_t *job_map = TLS_get_job_map();

    job_map->erase(job_entry.id);

    if (job_map->empty()) {
        delete job_map;
        job_map = nullptr;
    }
    TLS_set_job_map(job_map);
}

job_entry_t const & job_sentry_t::get_job_entry() {
    return job_entry;
}

signal_t *job_sentry_t::get_interruptor_signal()
{
    return &interruptor;
}

std::vector<job_wire_entry_t> get_job_wire_entries()
{
    std::vector<job_wire_entry_t> job_entries;
    pmap(get_num_threads(), [&](int32_t threadnum) {
        std::vector<job_entry_t> coro_job_entries;
        {
            on_thread_t thread((threadnum_t(threadnum)));
            job_map_t *job_map = TLS_get_job_map();
            if (job_map != nullptr) {
                for (auto const & job : *job_map) {
                    coro_job_entries.push_back(job.second->get_job_entry());
                }
            }
        }

        job_entries.insert(job_entries.end(),
                           coro_job_entries.begin(),
                           coro_job_entries.end());
    });

    return job_entries;
}
