// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "memcached/file.hpp"
#include "memcached/parser.hpp"
#include "concurrency/fifo_checker.hpp"

/* `file_memcached_interface_t` is a `memcached_interface_t` that reads queries
from a file and ignores the responses to its queries. */

class file_memcached_interface_t : public memcached_interface_t {

private:
    FILE *file;

public:
    explicit file_memcached_interface_t(const char *filename) :
        file(fopen(filename, "r"))
    { }
    ~file_memcached_interface_t() {
        fclose(file);
    }

    /* We throw away the responses */
    void write(UNUSED const char *buffer, UNUSED size_t bytes, UNUSED signal_t *interruptor) { }
    void write_unbuffered(UNUSED const char *buffer, UNUSED size_t bytes, UNUSED signal_t *interruptor) { }
    void flush_buffer(UNUSED signal_t *interruptor) { }
    bool is_write_open() { return false; }

    void read(void *buf, size_t nbytes, signal_t *interruptor) {
        if (interruptor->is_pulsed()) throw no_more_data_exc_t();
        if (fread(buf, nbytes, 1, file) == 0)
            throw no_more_data_exc_t();
    }

    void read_line(std::vector<char> *dest, signal_t *interruptor) {
        if (interruptor->is_pulsed()) throw no_more_data_exc_t();
        int limit = MEGABYTE;
        dest->clear();
        char c;
        const char *head = "\r\n";
        while ((*head) && ((c = getc(file)) != EOF) && (limit--) > 0) {
            dest->push_back(c);
            if (c == *head) {
                head++;
            } else {
                head = "\r\n";
            }
        }
        //we didn't every find a crlf unleash the exception
        if (*head) throw no_more_data_exc_t();
    }
};

void import_memcache(const char *filename, namespace_interface_t<memcached_protocol_t> *nsi, signal_t *interrupter) {
    guarantee(interrupter);
    interrupter->assert_thread();

    file_memcached_interface_t interface(filename);

    handle_memcache(&interface, nsi, MAX_CONCURRENT_QUEURIES_ON_IMPORT, NULL, interrupter);
}
