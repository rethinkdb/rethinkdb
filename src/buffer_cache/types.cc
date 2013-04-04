// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "buffer_cache/types.hpp"

sync_callback_t::~sync_callback_t() {
    wait();
}

void sync_callback_t::on_sync() {
    guarantee(necessary_sync_count_ > 0);
    --necessary_sync_count_;
    if (necessary_sync_count_ == 0) {
        pulse();
    }
}

void sync_callback_t::increase_necessary_sync_count(size_t amount) {
    guarantee(necessary_sync_count_ > 0);
    necessary_sync_count_ += amount;
}
