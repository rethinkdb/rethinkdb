// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "parsing/util.hpp"
#include "arch/io/network.hpp"

line_parser_t::line_parser_t(tcp_conn_t *_conn) : conn(_conn) {
    peek();
    bytes_read = 0;
}

// Returns a charslice to the next CRLF line in the TCP conn's buffer
// blocks until a full line is available
std::string line_parser_t::readLine(signal_t *closer) {
    while (!readCRLF(closer)) {
        bytes_read++;
    }

    // Construct a string of everything upto the CRLF
    std::string line(start_position, bytes_read - 2);
    pop(closer);
    return line;
}

std::string line_parser_t::readWord(signal_t *closer) {
    while (current(closer) != ' ') {
        bytes_read++;
    }

    std::string word(start_position, bytes_read);

    // Remove spaces to leave the conn in a clean state for the next call
    while (current(closer) == ' ') {
        bytes_read++;
    }
    pop(closer);
    return word;
}

void line_parser_t::peek() {
    const_charslice slice = conn->peek();
    start_position = slice.beg;
    end_position = slice.end;
}

void line_parser_t::pop(signal_t *closer) {
    conn->pop(bytes_read, closer);
    peek();
    bytes_read = 0;
}

char line_parser_t::current(signal_t *closer) {
    while (static_cast<int64_t>(bytes_read) >= (end_position - start_position)) {
        conn->read_more_buffered(closer);
        peek();
    }
    return start_position[bytes_read];
}

// Attempts to read a crlf at the current position
// possibily advanes the current position in its attempt to do so
bool line_parser_t::readCRLF(signal_t *closer) {
    if (current(closer) == '\r') {
        bytes_read++;

        if (current(closer) == '\n') {
            bytes_read++;
            return true;
        }
    }
    return false;
}

