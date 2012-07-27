#ifndef CONTAINERS_ARCHIVE_INTERRUPTIBLE_STREAM_HPP_
#define CONTAINERS_ARCHIVE_INTERRUPTIBLE_STREAM_HPP_

#include "containers/archive/archive.hpp"

#include "concurrency/signal.hpp"

class interruptible_read_stream_t : public read_stream_t {
  public:
    interruptible_read_stream_t() {}

    // Returns number of bytes read or 0 upon EOF, -1 upon error.
    //
    // If `interruptor` is non-NULL and pulses during execution of the read,
    // raises interrupted_exc_t and may invalidate the stream (ie. assume it
    // does unless you know it's a subclass that doesn't).
    virtual MUST_USE int64_t read_interruptible(void *p, int64_t n, signal_t *interruptor) = 0;

    // from read_stream_t.
    virtual int64_t read(void *p, int64_t n) { return read_interruptible(p, n, NULL); }

  protected:
    virtual ~interruptible_read_stream_t() {}
};

class interruptible_write_stream_t : public write_stream_t {
  public:
    interruptible_write_stream_t() {}

    // Returns n, or -1 upon error. Blocks until all bytes are written.
    //
    // If `interruptor` is non-NULL and pulses during execution of the write,
    // raises interrupted_exc_t and may invalidate the stream (ie. assume it
    // does unless you know it's a subclass that doesn't).
    virtual MUST_USE int64_t write_interruptible(const void *p, int64_t n, signal_t *interruptor) = 0;

    // from write_stream_t.
    virtual int64_t write(const void *p, int64_t n) { return write_interruptible(p, n, NULL); }

  protected:
    virtual ~interruptible_write_stream_t() {}
};

#endif // CONTAINERS_ARCHIVE_INTERRUPTIBLE_STREAM_HPP_
