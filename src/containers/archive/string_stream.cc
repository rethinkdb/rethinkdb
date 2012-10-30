// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <algorithm>

#include "containers/archive/string_stream.hpp"

read_string_stream_t::read_string_stream_t(const std::string &in) : str(in), pos(0) { }

int64_t read_string_stream_t::read(void *p, int64_t n) {
    n = std::min(n, static_cast<int64_t>(str.size()) - pos);

    str.copy(reinterpret_cast<char *>(p), n, pos);

    pos += n;

    return n;
}
