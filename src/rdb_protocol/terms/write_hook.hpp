// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TERMS_WRITE_HOOK_HPP_
#define RDB_PROTOCOL_TERMS_WRITE_HOOK_HPP_

#include <string>

#include "rdb_protocol/context.hpp"

// The declarations for the terms in write_hook.cc are in term.hpp
const char *const write_hook_blob_prefix = "$reql_write_hook_function$";

std::string format_write_hook_query(const write_hook_config_t &config);

#endif // RDB_PROTOCOL_TERMS_WRITE_HOOK_HPP_

