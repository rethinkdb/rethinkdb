#include "parsing/util.hpp"

#include <alloca.h>

#include "arch/io/network.hpp"

LineParser::LineParser(tcp_conn_t *conn_) : conn(conn_) {
    peek();
    bytes_read = 0;
}

// Returns a charslice to the next CRLF line in the TCP conn's buffer
// blocks until a full line is available
std::string LineParser::readLine() {
    while(!readCRLF()) {
        bytes_read++;
    }

    // Construct a string of everything upto the CRLF
    std::string line(start_position, bytes_read - 2);
    pop();
    return line;
}

// Both ensures that next line read is exactly bytes long and is able
// to operate more efficiently than readLine.
std::string LineParser::readLineOf(size_t bytes) {
    // TODO: Don't use alloca.

    // Since we know exactly how much we want to read we'll buffer ourselves
    // +2 is for expected terminating CRLF
    char *buff = static_cast<char *>(alloca(bytes + 2));
    conn->read(buff, bytes + 2);

    // Now we expect the line to be demarcated by a CRLF
    if((buff[bytes] == '\r') && (buff[bytes+1] == '\n')) {
        return std::string(buff, buff + bytes);
    } else {
        // Error: line incorrectly terminated.
        /* Note: Redis behavior seems to be to assume that the line is
           always correctly terminated. It will just take the rest of
           the input as the start of the next line and then probably
           end up in an error state. We can probably be more fastidious.
        */
        throw ParseError();
    }

    // No need to pop or anything. Did not use tcp_conn's buffer or bytes_read.
}

std::string LineParser::readWord() {
    while(current() != ' ') {
        bytes_read++;
    }

    std::string word(start_position, bytes_read);

    // Remove spaces to leave the conn in a clean state for the next call
    while(current() == ' ') {
        bytes_read++;
    }
    pop();
    return word;
}

void LineParser::peek() {
    const_charslice slice = conn->peek();
    start_position = slice.beg;
    end_position = slice.end;
}

void LineParser::pop() {
    conn->pop(bytes_read);
    peek();
    bytes_read = 0;
}

char LineParser::current() {
    while(bytes_read >= (end_position - start_position)) {
        conn->read_more_buffered();
        peek();
    }
    return start_position[bytes_read];
}

// Attempts to read a crlf at the current position
// possibily advanes the current position in its attempt to do so
bool LineParser::readCRLF() {
    if(current() == '\r') {
        bytes_read++;

        if(current() == '\n') {
            bytes_read++;
            return true;
        }
    }
    return false;
}

