#ifndef CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_

#include <stdint.h>

#include <string>

uint64_t get_max_total_cache_size();
uint64_t get_default_total_cache_size();
void log_warnings_for_cache_size(uint64_t);

/* Parses the contents of a /proc/meminfo file and estimates how much memory
is available for new applications, in bytes. Prefers the kernel's own
"MemAvailable" estimate (Linux 3.14 and newer); falls back to
"MemFree" + "Cached" + "Buffers" on older kernels. Returns false if the
contents could not be parsed reliably. Exposed for unit tests. */
bool parse_meminfo_file(const std::string &contents, uint64_t *mem_avail_out);

#endif  // CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_

