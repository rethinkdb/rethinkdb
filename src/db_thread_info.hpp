// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef DB_THREAD_INFO_HPP_
#define DB_THREAD_INFO_HPP_

#include "arch/runtime/runtime.hpp"

// The last thread is a service thread that runs an connection acceptor, a
// log writer, and possibly similar services, and does not run any db
// code (caches, serializers, etc). The reasoning is that when the
// acceptor (and possibly other utils) get placed on an event queue
// with the db code, the latency for these utils can increase
// significantly. In particular, it causes timeout bugs in clients
// that expect the acceptor to work faster.
inline int get_num_db_threads() {
    return get_num_threads() - 1;
}

#endif // DB_THREAD_INFO_HPP_

