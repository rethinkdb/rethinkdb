
#ifndef __DB_CPU_INFO_HPP__
#define __DB_CPU_INFO_HPP__

#include "arch/arch.hpp"

// The last CPU is a service CPU that runs an connection acceptor, a
// log writer, and possibly similar services, and does not run any db
// code (caches, serializers, etc). The reasoning is that when the
// acceptor (and possibly other utils) get placed on an event queue
// with the db code, the latency for these utils can increase
// significantly. In particular, it causes timeout bugs in clients
// that expect the acceptor to work faster.
inline int get_num_db_threads() {
    return get_num_threads() - 1;
}

#endif // __DB_CPU_INFO_HPP__

