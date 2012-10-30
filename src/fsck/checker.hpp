// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef FSCK_CHECKER_HPP_
#define FSCK_CHECKER_HPP_

#include <string>
#include <vector>

#include "arch/io/io_utils.hpp"
#include "utils.hpp"

namespace fsck {

/*
  Here are the checks that we perform.

  - that block_size divides extent_size divides filesize.  (size ratio
    check)

  - that all keys are in the appropriate slice.  (hash check)

  - that all keys in the correct order.  (order check)

  - that the tree is properly balanced.  (balance check)

  - that there are no disconnected nodes.  (connectedness check)

  - that large bufs have all the proper blocks.  (large buffer check)

  - that keys and small values are no longer than MAX_KEY_SIZE and
    MAX_IN_NODE_VALUE_SIZE, and that large values are no longer than
    MAX_VALUE_SIZE and longer than MAX_IN_NODE_VALUE_SIZE.  (size
    limit check)

  - that used block ids (including deleted block ids) within each
    slice are contiguous.  (contiguity check)

  - that all metablock CRCs match, except for zeroed out blocks.
    (metablock crc check)

  - that the metablock version numbers and serializer transaction ids
    grow monotonically from some starting point.  (metablock
    monotonicity check)

  - that blocks in use have greater block sequence ids than blocks not
    in use.  (supremacy check)

  - that the patches in the diff storage are sequentially numbered
*/

struct config_t {
    std::vector<std::string> input_filenames;
    std::string metadata_filename;
    std::string log_file_name;
    bool ignore_diff_log;

    bool print_command_line;
    bool print_file_version;
    io_backend_t io_backend;

    config_t() : ignore_diff_log(false), print_command_line(false), print_file_version(false), io_backend(aio_default) {}
};

bool check_files(const config_t *config);

}  // namespace fsck

#endif  // FSCK_CHECKER_HPP_
