#ifndef CONTAINERS_ARCHIVE_BUFFER_GROUP_STREAM_HPP_
#define CONTAINERS_ARCHIVE_BUFFER_GROUP_STREAM_HPP_

#include "containers/archive/archive.hpp"
#include "errors.hpp"

class const_buffer_group_t;

class buffer_group_read_stream_t : public read_stream_t {
public:
    explicit buffer_group_read_stream_t(const const_buffer_group_t *group);
    virtual ~buffer_group_read_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);

private:
    const const_buffer_group_t *group_;
    size_t bufnum_;
    int64_t bufpos_;

    DISABLE_COPYING(buffer_group_read_stream_t);
};



#endif  // CONTAINERS_ARCHIVE_BUFFER_GROUP_STREAM_HPP_
