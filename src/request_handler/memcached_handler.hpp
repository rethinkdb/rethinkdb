
#ifndef __MEMCACHED_HANDLER_HPP__
#define __MEMCACHED_HANDLER_HPP__

#include "request_handler/request_handler.hpp"
#include "btree/key_value_store.hpp"
#include "ctype.h"

class server_t;

/*! memcached_handler_t
 *  \brief Implements a wrapper for bin_memcached_handler_t and txt_memcached_handler_t
 *         which parses incoming packets to determine which type they are
 */
class memcached_handler_t :
    public request_handler_t
{
public:
    typedef request_handler_t::parse_result_t parse_result_t;
    using request_handler_t::conn_fsm;

public:
    explicit memcached_handler_t(server_t *server)
        : request_handler_t(), req_handler(NULL), server(server)
        {}
    ~memcached_handler_t() {
        if (req_handler)
            delete req_handler;
    }
    /*! parse_request
     *  \brief send the request down to the correct handler
     *  \param event the event to be parsed
     *  \return the results of the parsing
     */
    virtual parse_result_t parse_request(event_t *event);

private:

    request_handler_t *req_handler; // !< the correct memcached request handler
    server_t *server;
    
    /*! determine_protocol
     *  \brief determine which protocol is being used based on the first byte
     *  \param event an event containing a message using one of the protocols
     */
    void determine_protocol(const event_t *event);

private:
    bool memcached_stats(char *stat_key, char *result) {
        char *c = stat_key;
        
        //make things case insensitive
        while (isalpha(*c)) {
            *c = tolower(*c);
            c++;
        }
        
        if (*(c - 1) != 0)
            return false; /* !< we didn't reach a null terminator */

        if (strcmp(stat_key, "pid") == 0) {
            return true;
        } else if (strcmp(stat_key, "uptime") == 0) {
            return true;
        } else if (strcmp(stat_key, "time") == 0) {
            return true;
        } else if (strcmp(stat_key, "version") == 0) {
            return true;
        } else if (strcmp(stat_key, "pointer_size") == 0) {
            return true;
        } else if (strcmp(stat_key, "rusage_user") == 0) {
            return true;
        } else if (strcmp(stat_key, "rusage_system") == 0) {
            return true;
        } else if (strcmp(stat_key, "curr_items") == 0) {
            return true;
        } else if (strcmp(stat_key, "total_items") == 0) {
            return true;
        } else if (strcmp(stat_key, "bytes") == 0) {
            return true;
        } else if (strcmp(stat_key, "curr_connections") == 0) {
            return true;
        } else if (strcmp(stat_key, "total_connections") == 0) {
            return true;
        } else if (strcmp(stat_key, "connection_structures") == 0) {
            return true;
        } else if (strcmp(stat_key, "cmd_get") == 0) {
            return true;
        } else if (strcmp(stat_key, "cmd_set") == 0) {
            return true;
        } else if (strcmp(stat_key, "get_hits") == 0) {
            return true;
        } else if (strcmp(stat_key, "get_misses") == 0) {
            return true;
        } else if (strcmp(stat_key, "evictions") == 0) {
            return true;
        } else if (strcmp(stat_key, "bytes_read") == 0) {
            return true;
        } else if (strcmp(stat_key, "bytes_written") == 0) {
            return true;
        } else if (strcmp(stat_key, "limit_maxbytes") == 0) {
            return true;
        } else if (strcmp(stat_key, "threads") == 0) {
            return true;
        } else {
            return false;
        }
        return false;
    }
};

#endif // __MEMCACHED_HANDLER_HPP__
