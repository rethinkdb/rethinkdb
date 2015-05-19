// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_BASE64_HPP_
#define RDB_PROTOCOL_BASE64_HPP_

#include <string>

std::string encode_base64(const char *data, size_t size);
std::string decode_base64(const char *bdata, size_t bsize);

#endif  // RDB_PROTOCOL_BASE64_HPP_
