#ifndef COMPRESSION_READ_WRITE_STREAM_ADAPTER_HPP_
#define COMPRESSION_READ_WRITE_STREAM_ADAPTER_HPP_

#include <snappy-sinksource.h>
#include "containers/archive/archive.hpp"

class write_stream_sink_t : public snappy::Sink {
public:
    write_stream_sink_t(write_stream_t *write_stream);
    virtual void Append(const char *bytes, size_t n);
private:
    write_stream_t *internal;
};

#endif
