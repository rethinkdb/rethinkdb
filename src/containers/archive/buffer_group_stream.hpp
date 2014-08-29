// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_BUFFER_GROUP_STREAM_HPP_
#define CONTAINERS_ARCHIVE_BUFFER_GROUP_STREAM_HPP_

#include "version.hpp"
#include "containers/archive/archive.hpp"
#include "utils.hpp"

class const_buffer_group_t;
class buffer_group_t;

class buffer_group_read_stream_t : public read_stream_t {
public:
    explicit buffer_group_read_stream_t(const const_buffer_group_t *group);
    virtual ~buffer_group_read_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);

    bool entire_stream_consumed() const;

private:
    const const_buffer_group_t *group_;
    size_t bufnum_;
    int64_t bufpos_;

    DISABLE_COPYING(buffer_group_read_stream_t);
};

class buffer_group_write_stream_t : public write_stream_t {
public:
    explicit buffer_group_write_stream_t(const buffer_group_t *group);
    virtual ~buffer_group_write_stream_t();

    virtual MUST_USE int64_t write(const void *p, int64_t n);

    bool entire_stream_filled() const;

private:
    const buffer_group_t *group_;
    size_t bufnum_;
    int64_t bufpos_;

    DISABLE_COPYING(buffer_group_write_stream_t);
};

template <cluster_version_t W, class T>
void deserialize_from_group(const const_buffer_group_t *group, T *value_out) {
    buffer_group_read_stream_t stream(group);
    archive_result_t res = deserialize<W>(&stream, value_out);
    guarantee_deserialization(res, "T (from a buffer group)");
    guarantee(stream.entire_stream_consumed(),
              "Corrupted value in storage (deserialization terminated early).");
}

template <class T>
void deserialize_for_version_from_group(cluster_version_t cluster_version,
                                        const const_buffer_group_t *group,
                                        T *value_out) {
    buffer_group_read_stream_t stream(group);
    archive_result_t res = deserialize_for_version(cluster_version, &stream,
                                                   value_out);
    guarantee_deserialization(res, "T (from a buffer group)");
    guarantee(stream.entire_stream_consumed(),
              "Corrupted value in storage (deserialization terminated early).");
}


#endif  // CONTAINERS_ARCHIVE_BUFFER_GROUP_STREAM_HPP_
