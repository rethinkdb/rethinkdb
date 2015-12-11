// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/username.hpp"

#include "unicode/unistr.h"
#include "unicode/usprep.h"

#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"

namespace auth {

username_t::username_t() { }

username_t::username_t(std::string username)
    : m_username(std::move(username)) {
    // FIXME
}

std::string const &username_t::to_string() const {
    return m_username;
}

bool username_t::operator<(username_t const &rhs) const {
    return m_username < rhs.m_username;
}

bool username_t::operator==(username_t const &rhs) const {
    return m_username == rhs.m_username;
}

RDB_IMPL_SERIALIZABLE_1(
    username_t
  , m_username);
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(username_t);

}  // namespace auth
