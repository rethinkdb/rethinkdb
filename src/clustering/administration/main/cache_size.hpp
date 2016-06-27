#ifndef CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_

#include <stdint.h>

#include <string>

uint64_t get_used_swap();
uint64_t get_max_total_cache_size();
uint64_t get_default_total_cache_size();
void log_warnings_for_cache_size(uint64_t);

#endif  // CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_

