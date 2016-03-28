// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/password.hpp"

#include "containers/archive/stl_types.hpp"
#include "crypto/pbkcs5_pbkdf2_hmac.hpp"
#include "crypto/random.hpp"

namespace auth {

password_t::password_t() {
}

password_t::password_t(std::string const &password, uint32_t iteration_count)
    : m_iteration_count(iteration_count),
      m_salt(crypto::random_bytes<16>()),
      m_hash(crypto::pbkcs5_pbkdf2_hmac_sha256(password, m_salt, m_iteration_count)),
      m_is_empty(password == "") {
}

/* static */ password_t password_t::generate_password_for_unknown_user() {
    password_t password;
    password.m_iteration_count = password_t::default_iteration_count,
    password.m_salt = crypto::random_bytes<16>(),
    password.m_hash.fill('\0');
    return password;
}

uint32_t password_t::get_iteration_count() const {
    return m_iteration_count;
}

std::array<unsigned char, 16> const &password_t::get_salt() const {
    return m_salt;
}

std::array<unsigned char, SHA256_DIGEST_LENGTH> const &password_t::get_hash() const {
    return m_hash;
}

bool password_t::is_empty() const {
    return m_is_empty;
}

bool password_t::operator==(password_t const &rhs) const {
    return
        m_iteration_count == rhs.m_iteration_count &&
        m_salt == rhs.m_salt &&
        m_hash == rhs.m_hash &&
        m_is_empty == rhs.m_is_empty;
}

RDB_IMPL_SERIALIZABLE_4(
    password_t,
    m_iteration_count,
    m_salt,
    m_hash,
    m_is_empty);
INSTANTIATE_SERIALIZABLE_SINCE_v2_3(password_t);

}  // namespace auth
