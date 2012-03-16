#ifndef PARSING_UTIL_HPP_
#define PARSING_UTIL_HPP_

#include <string>
#include <alloca.h>
#include "arch/io/network.hpp"
#include "arch/types.hpp"
#include "utils.hpp"

class ParseError {
public:
    virtual ~ParseError() {}
    virtual const char *what() const throw() {
        return "parse failure";
    }
};

// Parses CRLF terminated lines from a TCP connection
class LineParser {
private:
    tcp_conn_t *conn;

    const char *start_position;
    unsigned bytes_read;
    const char *end_position;

public:
    explicit LineParser(tcp_conn_t *conn_);

    // Returns a charslice to the next CRLF line in the TCP conn's buffer
    // blocks until a full line is available
    std::string readLine(); 

    // Both ensures that next line read is exactly bytes long and is able
    // to operate more efficiently than readLine.
    std::string readLineOf(size_t bytes);

    // Reads a single space terminated word from the TCP conn
    std::string readWord();

private:
    void peek();
    void pop();
    char current();

    // Attempts to read a crlf at the current position
    // possibily advanes the current position in its attempt to do so
    bool readCRLF();
};

#endif /* PARSING_UTIL_HPP_ */
