// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_PASSWORD_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_PASSWORD_HPP

#include <array>
#include <string>

#include "errors.hpp"
#include <openssl/sha.h>

#include "rpc/serialize_macros.hpp"

namespace auth {

class password_t
{
public:
    password_t();
    password_t(std::string password, uint32_t iteration_count = 4096);

    bool operator==(password_t const &rhs) const;

    RDB_DECLARE_ME_SERIALIZABLE(password_t);

private:
    std::array<unsigned char, 16> m_salt;
    std::array<unsigned char, SHA256_DIGEST_LENGTH> m_hash;
    uint32_t m_iteration_count;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_PASSWORD_HPP
