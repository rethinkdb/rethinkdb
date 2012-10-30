// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_THICK_LIST_HPP_
#define CONTAINERS_THICK_LIST_HPP_

#include <algorithm>

#include <vector>
#include <set>

// This is a combination of a list with holes in it and a free list.
// Tokens are "alive" when you add values with add() and dead when you
// remove them with drop().  If you use get() with a dead token,
// you'll get the value T() back, so you can tell if a token is dead,
// if you never add a value equal to T(), such as a null pointer.
// add() must never have a value that equals T().  T should have a
// cheap default constructor and must also be copyable.  drop() _must_
// be called with a live token.

// We could say class token_t = typename std::vector<T>::size_type but
// so far we want everybody using this to use explicitly sized
// unsigned token_types like uint32_t.

template <class T, class token_t>
class thick_list {
public:
    typedef token_t token_type;
    typedef T value_type;

    thick_list() {
#ifndef NDEBUG
        // Asserts that token_t is unsigned.  Sure, there might be a
        // better way to do this.
        token_t value = 0;
        --value;
        rassert(value > 0);
#endif

    }

    // adds a value, returning a token_t (a numeric type) for
    // accessing it.  The expression value != T() _must_ be true.
    token_type add(T const& value) {
        rassert(value != T());

        if (free_.empty()) {
            token_t ret = values_.size();
            values_.push_back(value);
#ifndef NDEBUG
            is_free_.push_back(false);
#endif
            return ret;
        } else {
            token_t ret = free_.back();
            rassert(values_[ret] == T());
            rassert(is_free_[ret]);
            free_.pop_back();
            values_[ret] = value;
#ifndef NDEBUG
            is_free_[ret] = false;
#endif
            return ret;
        }
    }

    // adds a value, returning false if the given token is not
    // "reasonable".  A reasonable token is one not currently in use,
    // and if any previously used tokens are available, it must be one
    // of the previously used ones.  If no previously used tokens are
    // available, it must be the smallest unused token.  You'll get
    // good performance if you never choose a bad token and use the
    // most recently dropped token.
    bool add(token_type known_token, T const& value) {
        if (free_.empty()) {
            if (known_token == values_.size()) {
                values_.push_back(value);
#ifndef NDEBUG
                is_free_.push_back(false);
#endif
                return true;
            } else {
                return false;
            }
        } else {
            // If a replicand used add(), then it picked the token
            // from the bock of its free list.  So the replicant, the
            // presumed caller of this function, searches in reverse
            // order.
            typename std::vector<token_t>::reverse_iterator p = std::find(free_.rbegin(), free_.rend(), known_token);
            if (p == free_.rend()) {
                return false;
            }

            if (values_.size() > known_token) {
                free_.erase(p.base() - 1);
                values_[known_token] = value;
                rassert(is_free_[known_token]);
#ifndef NDEBUG
                is_free_[known_token] = false;
#endif

                return true;
            } else {
                return false;
            }
        }
    }

    // Returns a value for the given token, or T() if the token is not
    // alive.
    T operator[](token_type token) const {
        if (token < values_.size()) {
            return values_[token];
        } else {
            return T();
        }
    }

    // We can use operator[] with any value x where 0 <= x < end_index().
    token_type end_index() const {
        return values_.size();
    }

    // Drops a value for the given token.
    void drop(token_type token) {
        rassert(token < values_.size());
        rassert(!is_free_[token]);

        values_[token] = T();
        free_.push_back(token);
#ifndef NDEBUG
        is_free_[token] = true;
#endif
    }

private:
    std::vector<T> values_;
    std::vector<token_t> free_;

#ifndef NDEBUG
    std::vector<bool> is_free_;
#endif

    DISABLE_COPYING(thick_list);  // Copying is stupid.
};



#endif  // CONTAINERS_THICK_LIST_HPP_
