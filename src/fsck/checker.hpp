#ifndef __FSCK_CHECKER_HPP__
#define __FSCK_CHECKER_HPP__

#include <vector>

#include "utils.hpp"

namespace fsck {

/*

  Here is a superset of the checks we're going to perform.

  - that block_size divides extent_size divides filesize.  (size ratio
    check)

  - that all keys are in the appropriate slice.  (hash check)

  - that all keys in the correct order.  (order check)

  - that the tree is properly balanced.  (balance check)

  - that there are no disconnected nodes.  (connectedness check)

  - that deleted blocks have proper zerobufs.  (zeroblock check)

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

  - that blocks in use have greater transaction ids than blocks not
    in use.  (supremacy check)
*/



/*
  The general algorithm we use is straightforward and is as follows,
  except that checks for "magic" are implicit.
  
  * Read the file static config.
  
    - CHECK block_size divides extent_size divides file_size.

    - LEARN block_size, extent_size.

  * Read metablocks

    - CHECK metablock monotonicity.

    - LEARN lba_shard_metablock_t[0:LBA_SHARD_FACTOR].

  * Read the LBA shards

    - CHECK that 0 <= block_ids <= MAX_BLOCK_ID.

    - CHECK that each off64_t is in a real extent.

    - CHECK that each block id is in the proper shard.

    - LEARN offsets for each block id.

  * Read block 0 (the CONFIG_BLOCK_ID block)

    - LEARN n_files, n_slices, serializer_number.

  * *FOR EACH SLICE*

    * Read the btree_superblock_t.

      - CHECK database_exists.

      - LEARN root_block.

    * Walk tree

      - CHECK ordering, balance, hash function, large buf, size limit.

      - LEARN which blocks are in the tree.

      - LEARN transaction id

    * For each non-deleted block unused by btree

      - LEARN transaction id

      - REPORT error

    * For each deleted block

      - CHECK zerobuf.

      - LEARN transaction id

  * For each garbage block

    - CHECK ser transaction id inferiority

*/

struct fsck_config_t {
    std::vector<std_string_t, gnew_alloc<std_string_t> > input_filenames;
    std_string_t log_file_name;
};

void check_files(const fsck_config_t& config);



}  // namespace fsck



#endif  // __FSCK_CHECKER_HPP__
