// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_PASSWORD_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_PASSWORD_HPP

#include <openssl/sha.h>

#include <array>
#include <string>

#include "rpc/serialize_macros.hpp"

namespace auth {

class password_t
{
public:
    password_t();
    password_t(std::string const &password, uint32_t iteration_count = 4096);

    std::array<unsigned char, 16> const &get_salt() const;
    std::array<unsigned char, SHA256_DIGEST_LENGTH> const &get_hash() const;
    uint32_t get_iteration_count() const;

    bool operator==(password_t const &rhs) const;

    RDB_DECLARE_ME_SERIALIZABLE(password_t);

private:
    std::array<unsigned char, 16> m_salt;
    std::array<unsigned char, SHA256_DIGEST_LENGTH> m_hash;
    uint32_t m_iteration_count;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_PASSWORD_HPP
