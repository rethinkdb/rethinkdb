#ifndef __REPLICATION_VALUE_STREAM_HPP__
#define __REPLICATION_VALUE_STREAM_HPP__

namespace replication {

// Right now this is super-dumb and WILL need to be
// reimplemented/enhanced.  Right now this is enhanceable.

class value_stream_t {
    value_stream_t(const char *beg, const char *end);


private:
    value_stream_t

    std::string buffer;
};

}  // namespace replication

#endif  // __REPLICATION_VALUE_STREAM_HPP__
