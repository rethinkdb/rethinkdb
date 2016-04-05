// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/password.hpp"

#include "errors.hpp"
#include <boost/algorithm/string/join.hpp>

#include "clustering/administration/admin_op_exc.hpp"
#include "containers/archive/stl_types.hpp"
#include "crypto/pbkcs5_pbkdf2_hmac.hpp"
#include "crypto/random.hpp"

namespace auth {

password_t::password_t() {
}

password_t::password_t(std::string const &password, uint32_t iteration_count)
    : m_iteration_count(iteration_count),
      m_salt(crypto::random_bytes<salt_length>()),
      m_hash(crypto::pbkcs5_pbkdf2_hmac_sha256(password, m_salt, m_iteration_count)),
      m_is_empty(password.empty()) {
}

password_t::password_t(ql::datum_t const &datum) {
    if (datum.get_type() != ql::datum_t::R_OBJECT) {
        throw admin_op_exc_t(
            "Expected an object, got " + datum.print() + ".", query_state_t::FAILED);
    }

    std::set<std::string> keys;
    for (size_t i = 0; i < datum.obj_size(); ++i) {
        keys.insert(datum.get_pair(i).first.to_std());
    }

    ql::datum_t iterations = datum.get_field("iterations", ql::NOTHROW);
    if (!iterations.has()) {
        throw admin_op_exc_t("Expected a field named `iterations`.", query_state_t::FAILED);
    }
    if (iterations.get_type() != ql::datum_t::R_NUM || iterations.as_int() < 1) {
        throw admin_op_exc_t(
            "Expected iterations to be a number greater than zero, got " +
                iterations.print() + ".",
            query_state_t::FAILED);
    }
    keys.erase("iterations");
    m_iteration_count = iterations.as_int();

    m_salt = crypto::random_bytes<salt_length>();

    ql::datum_t password = datum.get_field("password", ql::NOTHROW);
    if (!password.has()) {
        throw admin_op_exc_t("Expected a field named `password`.", query_state_t::FAILED);
    }
    if (password.get_type() != ql::datum_t::R_STR) {
        throw admin_op_exc_t(
            "Expected `password` to be a string, got " + password.print() + ".",
            query_state_t::FAILED);
    }
    keys.erase("password");
    m_hash = crypto::pbkcs5_pbkdf2_hmac_sha256(
        password.as_str().to_std(), m_salt, m_iteration_count);
    m_is_empty = password.as_str().to_std().empty();

    if (!keys.empty()) {
        throw admin_op_exc_t(
            "Unexpected key(s) `" + boost::algorithm::join(keys, "`, `") + "`.",
            query_state_t::FAILED);
    }
}

/* static */ password_t password_t::generate_password_for_unknown_user() {
    password_t password;
    password.m_iteration_count = password_t::default_iteration_count,
    password.m_salt = crypto::random_bytes<salt_length>(),
    password.m_hash.fill('\0');
    password.m_is_empty = false;
    return password;
}

uint32_t password_t::get_iteration_count() const {
    return m_iteration_count;
}

std::array<unsigned char, password_t::salt_length> const &password_t::get_salt() const {
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
