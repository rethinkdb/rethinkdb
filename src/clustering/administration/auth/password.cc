// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/password.hpp"

#include "errors.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"

namespace auth {

password_t::password_t() {
}

password_t::password_t(std::string password, uint32_t iteration_count)
    : m_iteration_count(iteration_count) {
    if (RAND_bytes(m_salt.data(), m_salt.size()) != 1) {
        // FIXME, ERR_get_error
    }

    if (PKCS5_PBKDF2_HMAC(
            password.data(),
            password.size(),
            m_salt.data(),
            m_salt.size(),
            m_iteration_count,
            EVP_sha256(),
            m_hash.size(),
            m_hash.data()) != 1) {
        // FIXME
    }
}

bool password_t::operator==(password_t const &rhs) const {
    return
        m_hash == rhs.m_hash &&
        m_salt == rhs.m_salt &&
        m_iteration_count == rhs.m_iteration_count;
}

RDB_IMPL_SERIALIZABLE_3(
    password_t
  , m_hash
  , m_salt
  , m_iteration_count);
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(password_t);

}  // namespace auth
