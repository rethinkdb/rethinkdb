// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef PARSING_UTIL_HPP_
#define PARSING_UTIL_HPP_

#include <string>

#include "arch/types.hpp"
#include "concurrency/signal.hpp"
#include "utils.hpp"

// Parses CRLF terminated lines from a TCP connection
class line_parser_t {
private:
    tcp_conn_t *conn;

    const char *start_position;
    unsigned bytes_read;
    const char *end_position;

public:
    explicit line_parser_t(tcp_conn_t *conn_);

    // Returns a charslice to the next CRLF line in the TCP conn's buffer
    // blocks until a full line is available
    std::string readLine(signal_t *closer);

    // Reads a single space terminated word from the TCP conn
    std::string readWord(signal_t *closer);

private:
    void peek();
    void pop(signal_t *closer);
    char current(signal_t *closer);

    // Attempts to read a crlf at the current position
    // possibily advanes the current position in its attempt to do so
    bool readCRLF(signal_t *closer);
};

#endif /* PARSING_UTIL_HPP_ */
