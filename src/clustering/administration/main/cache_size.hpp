#ifndef CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_

#include <stdint.h>

#include <string>

uint64_t get_default_total_cache_size();

bool validate_total_cache_size(uint64_t total_cache_size, std::string *error_out);

/* Prints total cache size to the log file, and also prints warnings if the size is
unreasonable */
void log_total_cache_size(uint64_t total_cache_size);

#endif  // CLUSTERING_ADMINISTRATION_MAIN_CACHE_SIZE_HPP_

