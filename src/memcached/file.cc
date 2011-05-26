#include "memcached/file.hpp"
#include "memcached/memcached.hpp"
#include "progress/progress.hpp"

/* `file_memcached_interface_t` is a `memcached_interface_t` that reads queries
from a file and ignores the responses to its queries. */

class file_memcached_interface_t : public memcached_interface_t {

private:
    FILE *file;
    file_progress_bar_t progress_bar;

public:
    file_memcached_interface_t(std::string filename) :
        file(fopen(filename.c_str(), "r")), progress_bar(std::string("Import"), file)
    { }
    ~file_memcached_interface_t() {
        fclose(file);
    }

    /* We throw away the responses */
    void write(UNUSED const thread_saver_t& saver, UNUSED const char *buffer, UNUSED size_t bytes) { }
    void write_unbuffered(UNUSED const char *buffer, UNUSED size_t bytes) { }
    void flush_buffer() { }
    bool is_write_open() { return false; }

    void read(void *buf, size_t nbytes) {
        if (fread(buf, nbytes, 1, file) == 0)
            throw no_more_data_exc_t();
    }

    void read_line(std::vector<char> *dest) {
        int limit = MEGABYTE;
        dest->clear();
        char c; 
        const char *head = "\r\n";
        while ((*head) && ((c = getc(file)) != EOF) && (limit--) > 0) {
            dest->push_back(c);
            if (c == *head) head++;
            else head = "\r\n";
        }
        //we didn't every find a crlf unleash the exception 
        if (*head) throw no_more_data_exc_t();
    }
};

/* In our current use of import we ignore gets, the easiest way to do this
 * is with a dummyed get_store */
class dummy_get_store_t : public get_store_t {
    get_result_t get(UNUSED const store_key_t &key, UNUSED order_token_t token) { return get_result_t(); }
    rget_result_t rget(UNUSED rget_bound_mode_t left_mode, UNUSED const store_key_t &left_key,
                       UNUSED rget_bound_mode_t right_mode, UNUSED const store_key_t &right_key, 
                       UNUSED order_token_t token)
    {
        return rget_result_t();
    }
};

void import_memcache(std::string filename, set_store_interface_t *set_store, order_source_t *order_source) {
    dummy_get_store_t dummy_get_store;
    file_memcached_interface_t interface(filename);

    handle_memcache(&interface, &dummy_get_store, set_store, MAX_CONCURRENT_QUEURIES_ON_IMPORT, order_source);
}
