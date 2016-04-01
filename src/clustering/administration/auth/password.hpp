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
    static constexpr uint32_t default_iteration_count = 4096;
    static constexpr std::size_t salt_length = 16;

    password_t();
    explicit password_t(
        std::string const &password,
        uint32_t iteration_count = default_iteration_count);

    static password_t generate_password_for_unknown_user();

    uint32_t get_iteration_count() const;
    std::array<unsigned char, salt_length> const &get_salt() const;
    std::array<unsigned char, SHA256_DIGEST_LENGTH> const &get_hash() const;
    bool is_empty() const;

    bool operator==(password_t const &rhs) const;

    RDB_DECLARE_ME_SERIALIZABLE(password_t);

private:
    uint32_t m_iteration_count;
    std::array<unsigned char, salt_length> m_salt;
    std::array<unsigned char, SHA256_DIGEST_LENGTH> m_hash;
    bool m_is_empty;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_PASSWORD_HPP
