#include "buffer_cache/stats.hpp"

perfmon_counter_t
    pm_n_blocks_in_memory("blocks_in_memory[blocks]"),
    pm_n_blocks_dirty("blocks_dirty[blocks]"),
    pm_n_blocks_total("blocks_total[blocks]");

