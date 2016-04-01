// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CLIENT_PROTOCOL_ERROR_HPP
#define CLIENT_PROTOCOL_ERROR_HPP

#include <stdexcept>

namespace client_protocol {

class client_server_error_t : public std::runtime_error {
public:
    client_server_error_t(uint32_t error_code, std::string const &what)
        : std::runtime_error(what),
          m_error_code(error_code) {
    }

    uint32_t get_error_code() const {
        return m_error_code;
    }

private:
    uint32_t m_error_code;
};

}  // namespace client_protocol

#endif  // CLIENT_PROTOCOL_ERROR_HPP
