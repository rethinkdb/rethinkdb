// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CLIENT_PROTOCOL_CLIENT_SERVER_ERROR_HPP_
#define CLIENT_PROTOCOL_CLIENT_SERVER_ERROR_HPP_

#include <stdexcept>

namespace client_protocol {

class client_server_error_t : public std::runtime_error {
public:
    client_server_error_t(uint32_t error_code, std::string const &_what)
        : std::runtime_error(_what),
          m_error_code(error_code) {
    }

    uint32_t get_error_code() const {
        return m_error_code;
    }

private:
    uint32_t m_error_code;
};

}  // namespace client_protocol

#endif  // CLIENT_PROTOCOL_CLIENT_SERVER_ERROR_HPP_
