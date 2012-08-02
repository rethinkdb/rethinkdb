#ifndef __STRESS_CLIENT_PROTOCOLS_MEMCACHED_SOCK_PROTOCOL_HPP__
#define __STRESS_CLIENT_PROTOCOLS_MEMCACHED_SOCK_PROTOCOL_HPP__

#include <climits>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <vector>
#include <map>
#include "protocol.hpp"
#include <queue>

#define MAX_MC_KEY_SIZE 250

class non_existent_command_error_t : public protocol_error_t {
public:
    non_existent_command_error_t() : protocol_error_t("Nonexistent command sent") { }
    virtual ~non_existent_command_error_t() throw () { }
};

class client_error_t : public protocol_error_t {
public:
    client_error_t(const std::string& message) : protocol_error_t("Client error: " + message) { }
    virtual ~client_error_t() throw () { }
};

class server_error_t : public protocol_error_t {
public:
    server_error_t(const std::string& message) : protocol_error_t("Server error: " + message) { }
    virtual ~server_error_t() throw () { }
};

class temporary_server_error_t : public server_error_t {
public:
    temporary_server_error_t(const std::string& message) : server_error_t(message) { }
    virtual ~temporary_server_error_t() throw () { }
};

class tcp_buffer_t {
public:
    std::vector<char> buffer;
    size_t bytes_in_buffer;
    size_t first_buffer_byte;

public:
    tcp_buffer_t()
        : bytes_in_buffer(0), first_buffer_byte(0)
    { }
};

class memcached_response_t {
public:
    memcached_response_t(tcp_buffer_t &thread_buffer) : buffer(thread_buffer) {
        successful = false;
    }
    ~memcached_response_t() {
        //clean up the buffer
    }

    void read_from_socket(const int socketfd) { }
    
    bool successful;
    std::string failure_message;
    
protected:
    int socketfd;

    // Helper methods for parsing server responses
    const char* read_line(size_t& result_length) {
        bool gotR = false;
        
        size_t scanned_up_to_position = 0;
        while (true) {
            while (scanned_up_to_position < buffer.bytes_in_buffer) {
                if (buffer.buffer[buffer.first_buffer_byte + scanned_up_to_position] == '\r')
                    gotR = true;
                else if (gotR && buffer.buffer[buffer.first_buffer_byte + scanned_up_to_position] == '\n') {
                    result_length = scanned_up_to_position - 1;
                    char* char_buffer = &buffer.buffer[0];
                    const char* result = char_buffer + buffer.first_buffer_byte;
                    buffer.first_buffer_byte += scanned_up_to_position + 1;
                    buffer.bytes_in_buffer -= scanned_up_to_position + 1;
                    
                    if (buffer.first_buffer_byte > 4096) {
                        // Wrap around and reset buffer
                        memmove(char_buffer, char_buffer + buffer.first_buffer_byte, buffer.bytes_in_buffer);
                        buffer.first_buffer_byte = 0;
                    }
                    return result;
                }
                else {
                    gotR = false;
                }

                ++scanned_up_to_position;
            }

            buffer.buffer.resize(std::max(buffer.first_buffer_byte + buffer.bytes_in_buffer + 512, buffer.buffer.size()), '\0');
            char* char_buffer = &buffer.buffer[0];
            const ssize_t bytes_read = recv(socketfd, char_buffer + buffer.first_buffer_byte + buffer.bytes_in_buffer, 512, 0);
            if (bytes_read <= 0) {
                perror("Unable to read from socket");
                exit(-1);
            }
            buffer.bytes_in_buffer += bytes_read;
        }
    }

    /* maybe_read_line is like read_line except that it does not block on io
     * and thus doesn't always return a line. When no line is found it will
     * return NULL and result_length will be set to 0 */

    const char* maybe_read_line(size_t &result_length) {
        /* first do a non-blocking read */
        buffer.buffer.resize(std::max(buffer.first_buffer_byte + buffer.bytes_in_buffer + 512, buffer.buffer.size()), '\0');
        const ssize_t bytes_read = recv(socketfd, buffer.buffer.data() + buffer.first_buffer_byte + buffer.bytes_in_buffer, buffer.buffer.size() - (buffer.first_buffer_byte + buffer.bytes_in_buffer), MSG_DONTWAIT);
        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            //We're fine with that, just means there wasn't any data
        } else if (bytes_read <= 0) {
            //Not fine with this, this is a real error
            perror("Unable to read from socket");
            exit(-1);
        } else {
            //Normal succesful read in which we got some data
            buffer.bytes_in_buffer += bytes_read;
        }

        /* Now check to see if we've got a CRLF */
        size_t scanned_up_to_position = 0;
        bool gotR = false;

        while (scanned_up_to_position < buffer.bytes_in_buffer) {
            if (buffer.buffer[buffer.first_buffer_byte + scanned_up_to_position] == '\r')
                gotR = true;
            else if (gotR && buffer.buffer[buffer.first_buffer_byte + scanned_up_to_position] == '\n') {
                result_length = scanned_up_to_position - 1;
                char* char_buffer = &buffer.buffer[0];
                const char* result = char_buffer + buffer.first_buffer_byte;
                buffer.first_buffer_byte += scanned_up_to_position + 1;
                buffer.bytes_in_buffer -= scanned_up_to_position + 1;

                if (buffer.first_buffer_byte > 4096) {
                    // Wrap around and reset buffer
                    memmove(char_buffer, char_buffer + buffer.first_buffer_byte, buffer.bytes_in_buffer);
                    buffer.first_buffer_byte = 0;
                }

                return result;
            } else {
                gotR = false;
            }

            ++scanned_up_to_position;
        }

        //We have failed to find a CRLF
        result_length = 0;
        return NULL;
    }
    
    const char* read_n_bytes(const size_t n) {
        buffer.buffer.resize(std::max(buffer.first_buffer_byte + n, buffer.buffer.size()), '\0');
        char* char_buffer = &buffer.buffer[0];
        if (buffer.bytes_in_buffer < n) {
            // MSG_WAITALL makes recv wait until it actually receives the full n bytes
            const ssize_t bytes_read = recv(socketfd, char_buffer + buffer.first_buffer_byte + buffer.bytes_in_buffer, n - buffer.bytes_in_buffer, MSG_WAITALL);
            if (bytes_read < n - buffer.bytes_in_buffer) {
                perror("Unable to read from socket");
                exit(-1);
            }
            buffer.bytes_in_buffer += bytes_read;
        }
        
        const char* result = char_buffer + buffer.first_buffer_byte;
        buffer.first_buffer_byte += n;
        buffer.bytes_in_buffer -= n;
        
        return result;
    }
    
    bool does_match_at_position(const char* check_in, const char* check_for, const size_t check_in_length, const size_t at_position = 0) const {
        return strlen(check_for) + at_position <= check_in_length && strncmp(check_in + at_position, check_for, strlen(check_for)) == 0;
    }
    
    int string_to_int(const std::string& string) const {
        char* end;
        const char* c_string = string.c_str();
        long int result = strtol(c_string, &end, 10);
        
        if (*end != '\0' || result == LONG_MAX && errno == ERANGE)
            throw protocol_error_t("Illegal integer literal: " + string);
            
        return static_cast<int>(result);
    }
    
    void check_for_error_condition(const char* response_line, const size_t line_length) const {
        if (does_match_at_position(response_line, "ERROR", line_length))
            throw non_existent_command_error_t();
        else if (does_match_at_position(response_line, "CLIENT_ERROR", line_length))
            throw client_error_t(std::string(response_line, line_length).substr(std::string("CLIENT_ERROR").length() + 1));
        else if (does_match_at_position(response_line, "SERVER_ERROR", line_length)) {
            const std::string error_message = std::string(response_line, line_length).substr(std::string("SERVER_ERROR").length() + 1);
            // Check for special Membase temporary failure case
            if (error_message == "temporary failure" || error_message == "proxy downstream timeout" || error_message == "proxy write to downstream")
                throw temporary_server_error_t(error_message);
            else
                throw server_error_t(error_message);
        }
    }
    
protected:
    tcp_buffer_t &buffer;
};

class memcached_delete_response_t : public memcached_response_t {
public:
    memcached_delete_response_t(tcp_buffer_t &thread_buffer) : memcached_response_t(thread_buffer) { }

    void read_from_socket(const int socketfd) {
        this->socketfd = socketfd;
        
        size_t line_length = 0;
        const char* line = read_line(line_length);
        check_for_error_condition(line, line_length);
        
        successful = line_length == 7 && strncmp("DELETED", line, line_length) == 0;
        if (!successful)
            failure_message = std::string(line, line_length);
    }
};

class memcached_store_response_t : public memcached_response_t {
public:
    memcached_store_response_t(tcp_buffer_t &thread_buffer) : memcached_response_t(thread_buffer) { }

    void read_from_socket(const int socketfd) {
        this->socketfd = socketfd;
        
        size_t line_length = 0;
        const char* line = read_line(line_length);
        check_for_error_condition(line, line_length);
        
        successful = line_length == 6 && strncmp("STORED", line, line_length) == 0;
        if (!successful)
            failure_message = std::string(line, line_length);
    }
};

class memcached_retrieval_response_t : public memcached_response_t {
public:
    memcached_retrieval_response_t(tcp_buffer_t &thread_buffer, const unsigned int number_of_keys, const bool mock_parse) : memcached_response_t(thread_buffer) {
        this->number_of_keys = number_of_keys;
        this->mock_parse = mock_parse;
    }

    virtual void read_from_socket(const int socketfd) {
        this->socketfd = socketfd;
        
        values.clear();
        flags.clear();
        unsigned int number_of_failed_gets = 0;
        
        for (unsigned int values_left = number_of_keys; values_left > 0; --values_left) {
            size_t line_length = 0;
            const char* line = read_line(line_length);
            check_for_error_condition(line, line_length); // We might still get ERROR...
            
            const std::string value_response(line, line_length);
            
            if (value_response == "END") {
                number_of_failed_gets = number_of_keys - values.size();
                break;
            }
            else if (does_match_at_position(line, "VALUE ", line_length, 0)) {
                if (mock_parse) {
                    parse_value_response_mock(value_response);
                } else {
                    parse_value_response(value_response);
                }
            }
            else {
                throw protocol_error_t("Illegal response to get request: " + value_response);
            }
        }
        
        if (number_of_failed_gets == 0) {
            size_t line_length = 0;
            const char* end_line = read_line(line_length);
            if (line_length != 3 || strncmp(end_line, "END", line_length) != 0)
                throw protocol_error_t("Did not get END at end of retrieval response.");
        }
        
        successful = number_of_failed_gets == 0;
        char number_of_failed_gets_c_str[32];
        sprintf(number_of_failed_gets_c_str, "%d", number_of_failed_gets);
        failure_message = successful ? "" : "Failed to read a total of " + std::string(number_of_failed_gets_c_str) + " values.";
    }
    
    bool mock_parse;
    std::map<std::string, std::string> values;
    std::map<std::string, int> flags;
    
protected:
    void parse_value_response(const std::string& value_response) {
        size_t parse_position = 6;
                
        const size_t key_end_position = value_response.find(' ', parse_position);
        if (key_end_position == value_response.npos)
            throw protocol_error_t("Unable to retrieve key from get response.");
        const std::string key = value_response.substr(parse_position, key_end_position - parse_position);
        parse_position += key_end_position - parse_position + 1;
        
        const size_t flags_end_position = value_response.find(' ', parse_position);
        if (flags_end_position == value_response.npos)
            throw protocol_error_t("Unable to retrieve flags from get response.");
        const std::string flags_string = value_response.substr(parse_position, flags_end_position - parse_position);
        parse_position += flags_end_position - parse_position + 1;
        
        const std::string size_string = value_response.substr(parse_position);
        
        const int flags = string_to_int(flags_string);
        const int size = string_to_int(size_string);
        
        const char* value = read_n_bytes(static_cast<size_t>(size));
        const std::string value_str(value, static_cast<size_t>(size));
        
        size_t line_length = 0;
        read_line(line_length);
        if (line_length > 0)
            throw protocol_error_t("Did not get line break after value.");

        values.insert(std::pair<std::string, std::string>(key, value_str));
        this->flags.insert(std::pair<std::string, int>(key, flags));
    }
    
    void parse_value_response_mock(const std::string& value_response) {
        size_t parse_position = 6;

        const size_t key_end_position = value_response.find(' ', parse_position);
        if (key_end_position == value_response.npos)
            throw protocol_error_t("Unable to retrieve key from get response.");
        parse_position += key_end_position - parse_position + 1;
        
        const size_t flags_end_position = value_response.find(' ', parse_position);
        if (flags_end_position == value_response.npos)
            throw protocol_error_t("Unable to retrieve flags from get response.");
        parse_position += flags_end_position - parse_position + 1;
        
        const std::string size_string = value_response.substr(parse_position);
        const int size = string_to_int(size_string);
        
        const char* value = read_n_bytes(static_cast<size_t>(size));
        
        size_t line_length = 0;
        read_line(line_length);
        if (line_length > 0)
            throw protocol_error_t("Did not get line break after value.");
    }

    unsigned int number_of_keys;
};

struct memcached_sock_protocol_t : public protocol_t {
    memcached_sock_protocol_t(const char *conn_str) 
        : sockfd(-1), outstanding_reads(0)
    {
        // init the socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            int err = errno;
            fprintf(stderr, "Could not create socket\n");
            exit(-1);
        }

        // Should be enough:
        thread_buffer.buffer.resize(1024 * 8, '\0');

        // Parse the host string
        char _host[MAX_HOST];
        strncpy(_host, conn_str, MAX_HOST);

        int port;
        if (char *_port = strchr(_host, ':')) {
            *_port = '\0';
            _port++;
            port = atoi(_port);
            if (port == 0) {
                fprintf(stderr, "Cannot parse port string: \"%s\".\n", _port);
                exit(-1);
            }
        } else {
            fprintf(stderr, "Please use host string of the form host:port.\n");
            exit(-1);
        }

        // Setup the host/port data structures
        struct sockaddr_in sin;
        struct hostent *host = gethostbyname(_host);
        if (!host) {
            herror("Could not gethostbyname()");
            exit(-1);
        }
        memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        // Connect to server
        int res = ::connect(sockfd, (struct sockaddr *)&sin, sizeof(sin));
        if (res < 0) {
            int err = errno;
            fprintf(stderr, "Could not connect to server (%d)\n", err);
            exit(-1);
        }
    }

    virtual ~memcached_sock_protocol_t() {
        // assert(!exist_outstanding_pipeline_reads());;
        if (sockfd != -1) {
            int res = close(sockfd);
            int err = errno;
            if (res != 0) {
                fprintf(stderr, "Could not close socket\n");
                exit(-1);
            }
        }
    }

    void send_buffer_at_least(int size) {
        if ((int)send_buffer.size() < size) send_buffer.resize(size);
    }

    virtual void remove(const char *key, size_t key_size) {
        assert(!exist_outstanding_pipeline_reads());;
        // Setup the text command
        send_buffer_at_least(key_size + 1024);
        char *buf = send_buffer.data();
        buf += sprintf(buf, "delete ");
        memcpy(buf, key, key_size);
        buf += key_size;
        buf += sprintf(buf, "\r\n");

        memcached_delete_response_t response(thread_buffer);

        retry:
        // Send it on its merry way to the server
        send_command(buf - send_buffer.data());

        // Parse the response
        try {
            response.read_from_socket(sockfd);
        } catch (temporary_server_error_t& e) {
            // Sleep a moment, then retry
            usleep(1000);
            goto retry;
        }

        // Check the result
        if (!response.successful) {
            //fprintf(stderr, "Failed to remove key %s: %s\n", key, response.failure_message.c_str());
        }
    }

    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());;
        insert(key, key_size, value, value_size);
    }

    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());;
        // Setup the text command
        send_buffer_at_least(key_size + value_size + 1024);
        char *buf = send_buffer.data();
        buf += sprintf(buf, "set ");
        memcpy(buf, key, key_size);
        buf += key_size;
        buf += sprintf(buf, " 0 0 %d\r\n", (int)value_size);
        memcpy(buf, value, value_size);
        buf += value_size;
        buf += sprintf(buf, "\r\n");

        memcached_store_response_t response(thread_buffer);

        retry:
        // Send it on its merry way to the server
        send_command(buf - send_buffer.data());

        // Parse the response
        try {
            response.read_from_socket(sockfd);
        } catch (temporary_server_error_t& e) {
            // Sleep a moment, then retry
            usleep(1000);
            goto retry;
        }


        // Check the result
        if (!response.successful) {
            //fprintf(stderr, "Failed to store key %s: %s\n", key, response.failure_message.c_str());
        }
    }

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());;
        // Setup the text command
        send_buffer_at_least(count * MAX_MC_KEY_SIZE + 1024);
        char *buf = send_buffer.data();
        buf += sprintf(buf, "get ");
        for(int i = 0; i < count; i++) {
            memcpy(buf, keys[i].first, keys[i].second);
            buf += keys[i].second;
            if (i != count - 1) {
                buf += sprintf(buf, " ");
            }
        }
        buf += sprintf(buf, "\r\n");

        memcached_retrieval_response_t response(thread_buffer, count, !values);

        retry:
        // Send it on its merry way to the server
        send_command(buf - send_buffer.data());

        // Parse the response
        try {
            response.read_from_socket(sockfd);
        } catch (temporary_server_error_t& e) {
            // Sleep a moment, then retry
            usleep(1000);
            goto retry;
        }

        // Check the result
        if (!response.successful) {
            //fprintf(stderr, "Failed to read: %s\n", response.failure_message.c_str());
        }

        if (values) {
            for (int i = 0; i < count; i++) {
                if (std::string(values[i].first) != response.values[std::string(keys[i].first)]) {
                    fprintf(stderr, "Got unexpected value: %s instead of %s\n", response.values[std::string(keys[i].first)].c_str(), values[i].first);
                }
            }
        }
    }

    int outstanding_reads;

    /* add a read to the pipeline */
    void enqueue_read(payload_t *keys, int count, payload_t *values = NULL) {
        // Setup the text command
        send_buffer_at_least(count * MAX_MC_KEY_SIZE + 1024);
        char *buf = send_buffer.data();
        buf += sprintf(buf, "get ");
        for(int i = 0; i < count; i++) {
            memcpy(buf, keys[i].first, keys[i].second);
            buf += keys[i].second;
            if (i != count - 1) {
                buf += sprintf(buf, " ");
            }
        }
        buf += sprintf(buf, "\r\n");

        send_command(buf - send_buffer.data());
        outstanding_reads++;
    }

    bool dequeue_read_maybe(payload_t *keys, int count, payload_t *values = NULL) {
        dequeue_read(keys, count, values);
        return true;
    }

    /* Wait until all of the pipelined reads have been returned */
    void dequeue_read(payload_t *keys, int count, payload_t *values = NULL) {
        memcached_retrieval_response_t response(thread_buffer, count, !values);
        response.read_from_socket(sockfd);

        if (values) {
            for (int i = 0; i < count; i++) {
                if (std::string(values[i].first) != response.values[std::string(keys[i].first)]) {
                    fprintf(stderr, "Got unexpected value: %s instead of %s\n", response.values[std::string(keys[i].first)].c_str(), values[i].first);
                }
            }
        }
        outstanding_reads--;
    }

    bool exist_outstanding_pipeline_reads() {
        return outstanding_reads != 0;
    }

    virtual void range_read(char* lkey, size_t lkey_size, char* rkey, size_t rkey_size, int count_limit, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());
        // Setup the text command
        send_buffer_at_least(lkey_size + rkey_size + 1024);
        char *buf = send_buffer.data();
        buf += sprintf(buf, "rget ");
        memcpy(buf, lkey, lkey_size);
        buf += lkey_size;
        buf += sprintf(buf, " ");
        memcpy(buf, rkey, rkey_size);
        buf += rkey_size;
        buf += sprintf(buf, " 0 0 %d", count_limit);
        buf += sprintf(buf, "\r\n");

        memcached_retrieval_response_t response(thread_buffer, count_limit, !values);

        retry:
        // Send it on its merry way to the server
        send_command(buf - send_buffer.data());

        // Parse the response
        try {
            response.read_from_socket(sockfd);
        } catch (temporary_server_error_t& e) {
            // Sleep a moment, then retry
            usleep(1000);
            goto retry;
        }

        // Check the result
        if (!response.successful) {
            //fprintf(stderr, "Failed to read: %s\n", response.failure_message.c_str());
        }

        if (values) {
            fprintf(stderr, "Value verification not implemented for range reads\n");
        }
    }

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());;
        // Setup the text command
        send_buffer_at_least(key_size + value_size + 1024);
        char *buf = send_buffer.data();
        buf += sprintf(buf, "append ");
        memcpy(buf, key, key_size);
        buf += key_size;
        buf += sprintf(buf, " 0 0 %d\r\n", (int)value_size);
        memcpy(buf, value, value_size);
        buf += value_size;
        buf += sprintf(buf, "\r\n");

        memcached_store_response_t response(thread_buffer);

        retry:
        // Send it on its merry way to the server
        send_command(buf - send_buffer.data());

        // Parse the response
        try {
            response.read_from_socket(sockfd);
        } catch (temporary_server_error_t& e) {
            // Sleep a moment, then retry
            usleep(1000);
            goto retry;
        }

        // Check the result
        if (!response.successful) {
            //fprintf(stderr, "Failed to store key %s (append): %s\n", key, response.failure_message.c_str());
        }
    }

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());;
        // Setup the text command
        send_buffer_at_least(key_size + value_size + 1024);
        char *buf = send_buffer.data();
        buf += sprintf(buf, "prepend ");
        memcpy(buf, key, key_size);
        buf += key_size;
        buf += sprintf(buf, " 0 0 %d\r\n", (int)value_size);
        memcpy(buf, value, value_size);
        buf += value_size;
        buf += sprintf(buf, "\r\n");

        memcached_store_response_t response(thread_buffer);

        retry:
        // Send it on its merry way to the server
        send_command(buf - send_buffer.data());

        // Parse the response
        try {
            response.read_from_socket(sockfd);
        } catch (temporary_server_error_t& e) {
            // Sleep a moment, then retry
            usleep(1000);
            goto retry;
        }

        // Check the result
        if (!response.successful) {
            //fprintf(stderr, "Failed to store key %s (prepend): %s\n", key, response.failure_message.c_str());
        }
    }

private:
    void send_command(int total) {
        int count = 0, res = 0;
        do {
            res = write(sockfd, send_buffer.data() + count, total - count);
            if (res < 0) {
                int err = errno;
                fprintf(stderr, "Could not send command (%d)\n", errno);
                exit(-1);
            }
            count += res;
        } while(count < total);
    }

private:
    int sockfd;
    std::vector<char> send_buffer;
    tcp_buffer_t thread_buffer;
};


#endif  // __STRESS_CLIENT_PROTOCOLS_MEMCACHED_SOCK_PROTOCOL_HPP__

