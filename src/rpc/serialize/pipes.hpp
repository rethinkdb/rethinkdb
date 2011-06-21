#ifndef __RPC_SERIALIZE_PIPES_HPP__
#define __RPC_SERIALIZE_PIPES_HPP__

#include "utils.hpp"

/* `cluster_outpipe_t` and `cluster_inpipe_t` are passed to user-code that's supposed to serialize
and deserialize cluster messages. User-code should use them by calling `write()` and `read()`. */

struct cluster_outpipe_t {
    virtual void write(const void *buf, size_t size) = 0;
    virtual ~cluster_outpipe_t() { }
};

struct cluster_inpipe_t {
    virtual void read(void *buf, size_t size) = 0;
    virtual ~cluster_inpipe_t() { }
};

/* `counting_outpipe_t` is like `cluster_outpipe_t` except that it just determines what the
eventual byte count will be without actually collecting the bytes that are written. */

struct counting_outpipe_t : public cluster_outpipe_t {
    counting_outpipe_t() : bytes(0) { }
    void write(UNUSED const void *buf, size_t size) {
        bytes += size;
    }
    int bytes;
};

/* Implementation code should typically subclass `checking_outpipe_t` and `checking_inpipe_t`
instead of `cluster_outpipe_t` and `cluster_inpipe_t`, and override `do_write()` and `do_read()`,
so that it can get automatic verification that the right number of bytes have been written
or read. */

struct checking_outpipe_t : public cluster_outpipe_t {
    void write(const void *buf, size_t size) {
        written += size;
        if (written > expected) {
            crash("counting_outpipe_t counted %d bytes, but serialize() is trying to write more.",
                expected);
        }
        do_write(buf, size);
    }
protected:
    checking_outpipe_t(int expected) : written(0), expected(expected) { }
    virtual ~checking_outpipe_t() {
        if (written != expected) {
            crash("counting_outpipe_t counted %d bytes, but serialize() only wrote %d.",
                expected, written);
        }
    }
    virtual void do_write(const void *, size_t) = 0;
    int written;   // How many bytes we've written so far
    int expected;   // How long the message is supposed to be
};

struct checking_inpipe_t : public cluster_inpipe_t {
    void read(void *buf, size_t size) {
        readed += size;
        if (readed > expected) {
            crash("The message was %d bytes long, but unserialize() is trying to read more.", expected);
        }
        do_read(buf, size);
    }
protected:
    checking_inpipe_t(int expected) : readed(0), expected(expected) { }
    virtual ~checking_inpipe_t() {
        if (readed != expected) {
            crash("The message was %d bytes long, but unserialize() only read %d.",
                expected, readed);
        }
    }
    virtual void do_read(void *, size_t) = 0;
    int readed;   // How many bytes we've read so far
    int expected;   // How long the message is supposed to be
};

#endif /* __RPC_SERIALIZE_PIPES_HPP__ */
