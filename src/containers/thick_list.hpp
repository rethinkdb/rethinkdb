#ifndef __CONTAINERS_THICK_LIST_HPP__
#define __CONTAINERS_THICK_LIST_HPP__

#include <algorithm>

#include <vector>

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
        -- value;
        rassert(value > 0);
#endif

    }

    // adds a value, returning a token_t (a numeric type) for
    // accessing it.  The expression value != T() _must_ be true.
    token_type add(T& value) {
        rassert(value != T());

        if (free_.empty()) {
            token_t ret = values_.size();
            values_.push_back(value);
        } else {
            token_t ret = free_.back();
            rassert(values_[ret] != T());
            free_.pop_back();
            values_[ret] = value;
        }
    }

    // adds a value, returning false if the given token is not
    // "reasonable".  A reasonable token is one not currently in use,
    // and if any previously used tokens are available, it must be one
    // of the previously used ones.  If no previously used tokens are
    // available, it must be the smallest unused token.  You'll get
    // good performance if you never choose a bad token and use the
    // most recently dropped token.

    // TODO: Consider throwing an exception, instead of returning false.
    bool add(token_type known_token, T& value) {
        if (free_.empty()) {
            if (known_token == values_.size()) {
                values_.push_back(value);
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

            free_.erase(p.base());
            return true;
        }
    }

    // Returns a value for the given token, or T() if the token is not
    // alive.
    T operator[](token_type token) {
        if (token < values_.size()) {
            return values_[token];
        } else {
            return T();
        }
    }

    // Drops a value for the given token.
    void drop(token_type token) {
        rassert(token < values_.size());
        rassert(free_.end() == std::find(free_.begin(), free_.end(), token));

        values_[token] = T();
        free_.push_back(token);
    }

private:
    DISABLE_COPYING(thick_list);  // Copying is stupid.

    std::vector<T> values_;
    std::vector<token_t> free_;
};



#endif  // __CONTAINERS_THICK_LIST_HPP__
