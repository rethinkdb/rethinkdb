// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CRYPTO_INITIALIZATION_GUARD_HPP
#define CRYPTO_INITIALIZATION_GUARD_HPP

namespace crypto {

class initialization_guard_t {
public:
    initialization_guard_t();
    ~initialization_guard_t();
};

}  // namespace crypto

#endif  // CRYPTO_INITIALIZATION_GUARD_HPP
