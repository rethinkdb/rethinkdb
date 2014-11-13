#ifndef CLUSTERING_ADMINISTRATION_MAIN_MEMINFO_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_MEMINFO_HPP_

#include <stdint.h>

#ifndef __MACH__

// Returns false if it can't get the available memory size.
bool get_proc_meminfo_available_memory_size(uint64_t *mem_avail_out);

#endif  // __MACH__

#endif  // CLUSTERING_ADMINISTRATION_MAIN_MEMINFO_HPP_

