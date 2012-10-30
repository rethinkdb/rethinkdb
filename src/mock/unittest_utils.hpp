// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MOCK_UNITTEST_UTILS_HPP_
#define MOCK_UNITTEST_UTILS_HPP_

#include "errors.hpp"
#include <boost/function.hpp>

#include "containers/scoped.hpp"
#include "rpc/serialize_macros.hpp"

namespace mock {

class temp_file_t {
public:
    explicit temp_file_t(const char *tmpl);
    const char *name() { return filename.data(); }
    ~temp_file_t();

private:
    scoped_array_t<char> filename;

    DISABLE_COPYING(temp_file_t);
};

void let_stuff_happen();

int randport();

void run_in_thread_pool(const boost::function<void()>& fun, int num_workers = 1);

}  // namespace mock

namespace unittest {

class sl_int_t {
public:
    sl_int_t() { }
    explicit sl_int_t(uint64_t initial) : i(initial) { }
    uint64_t i;

    RDB_MAKE_ME_SERIALIZABLE_1(i);
};

inline void semilattice_join(sl_int_t *a, sl_int_t b) {
    a->i |= b.i;
}

}  // namespace unittest

#endif /* MOCK_UNITTEST_UTILS_HPP_ */
