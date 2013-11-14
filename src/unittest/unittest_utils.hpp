// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_UNITTEST_UTILS_HPP_
#define UNITTEST_UNITTEST_UTILS_HPP_

#include <set>
#include <string>

#include "errors.hpp"
#include <boost/function.hpp>

#include "arch/address.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/serialize_macros.hpp"

namespace unittest {

serializer_filepath_t make_unittest_filepaths(const std::string &permanent_path,
                                              const std::string &temporary_path);

class temp_file_t {
public:
    temp_file_t();
    ~temp_file_t();
    serializer_filepath_t name() const;

private:
    std::string filename;

    DISABLE_COPYING(temp_file_t);
};

void let_stuff_happen();

std::set<ip_address_t> get_unittest_addresses();

void run_in_thread_pool(const boost::function<void()>& fun, int num_workers = 1);

rdb_protocol_t::read_t make_sindex_read(
    counted_t<const ql::datum_t> key, const std::string &id);

}  // namespace unittest

#endif /* UNITTEST_UNITTEST_UTILS_HPP_ */
