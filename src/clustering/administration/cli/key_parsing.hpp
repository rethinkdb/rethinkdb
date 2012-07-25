#ifndef CLUSTERING_ADMINISTRATION_CLI_KEY_PARSING_HPP_
#define CLUSTERING_ADMINISTRATION_CLI_KEY_PARSING_HPP_

#include <string>

#include "btree/keys.hpp"

bool cli_str_to_key(const std::string &str, store_key_t *out);
std::string key_to_cli_str(const store_key_t &key);

std::string key_range_to_cli_str(const key_range_t &range);

#endif /* CLUSTERING_ADMINISTRATION_CLI_KEY_PARSING_HPP_ */
