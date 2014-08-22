// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_DEBUG_DEBUG_SCRATCH_HPP_
#define CLUSTERING_ADMINISTRATION_DEBUG_DEBUG_SCRATCH_HPP_

#include <map>
#include <string>

#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/datum.hpp"

/* This is the backend for an artificial table that acts as much as possible like a real
table. It accepts all reads and writes, storing the results in a `std::map`. It's used
for testing `artificial_table_t`. */

class in_memory_artificial_table_backend_t :
    public artificial_table_backend_t
{
public:
    std::string get_primary_key_name() {
        return "id";
    }

    bool read_all_primary_keys(
            signal_t *interruptor,
            std::vector<counted_t<const ql::datum_t> > *keys_out,
            UNUSED std::string *error_out) {
        block(interruptor);
        on_thread_t thread_switcher(home_thread());
        keys_out->clear();
        for (auto it = data.begin(); it != data.end(); ++it) {
            counted_t<const ql::datum_t> key = it->second->get("id", ql::NOTHROW);
            guarantee(key.has());
            keys_out->push_back(key);
        }
        return true;
    }

    bool read_row(
            counted_t<const ql::datum_t> primary_key,
            signal_t *interruptor,
            counted_t<const ql::datum_t> *row_out,
            UNUSED std::string *error_out) {
        block(interruptor);
        on_thread_t thread_switcher(home_thread());
        auto it = data.find(primary_key->print_primary());
        if (it != data.end()) {
            *row_out = it->second;
        } else {
            *row_out = counted_t<const ql::datum_t>();
        }
        return true;
    }

    bool write_row(
            counted_t<const ql::datum_t> primary_key,
            counted_t<const ql::datum_t> new_value,
            signal_t *interruptor,
            UNUSED std::string *error_out) {
        block(interruptor);
        on_thread_t thread_switcher(home_thread());
        if (new_value.has()) {
            data[primary_key->print_primary()] = new_value;
        } else {
            data.erase(primary_key->print_primary());
        }
        return true;
    }

private:
    void block(signal_t *interruptor) {
        signal_timer_t timer;
        timer.start(randint(100));
        wait_interruptible(&timer, interruptor);
    }

    std::map<std::string, counted_t<const ql::datum_t> > data;
};

#endif /* CLUSTERING_ADMINISTRATION_DEBUG_DEBUG_SCRATCH_HPP_ */

