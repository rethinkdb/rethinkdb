// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/archive/string_stream.hpp"

string_stream_t::string_stream_t() { }

string_stream_t::~string_stream_t() { }

int64_t string_stream_t::write(const void *p, int64_t n) {
    const char * chp = static_cast<const char *>(p);
    str_.insert(str_.end(), chp, chp + n);
    return n;
}

string_read_stream_t::string_read_stream_t(std::string &&_source, int64_t _offset) :
    source(std::move(_source)), offset(_offset) {
    guarantee(offset >= 0);
    guarantee(static_cast<uint64_t>(offset) <= source.size());
}

string_read_stream_t::~string_read_stream_t() { }

int64_t string_read_stream_t::read(void *p, int64_t n) {
    int64_t num_left = source.size() - offset;
    int64_t num_to_read = n < num_left ? n : num_left;

    memcpy(p, source.data() + offset, num_to_read);

    offset += num_to_read;

    return num_to_read;
}

void string_read_stream_t::swap(std::string *other_source, int64_t *other_offset) {
    int64_t temp_offset = *other_offset;
    *other_offset = offset;
    offset = temp_offset;

    source.swap(*other_source);

    guarantee(offset >= 0);
    guarantee(static_cast<uint64_t>(offset) <= source.size());
}
