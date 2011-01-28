#ifndef __CONTAINERS_THICK_LIST_HPP__
#define __CONTAINERS_THICK_LIST_HPP__

#ifndef NDEBUG
#include <algorithm>
#endif  // NDEBUG

#include <vector>

// This is a combination of a list with holes in it and a free list.
// Tokens are "alive" when you add values with add() and dead when you
// remove them with drop().  If you use get() with a dead token,
// you'll get the value T() back, so you can tell if a token is dead,
// if you never add a value equal to T(), such as a null pointer.
// add() must never have a value that equals T().  T should have a
// cheap default constructor and must also be copyable.  drop() _must_
// be called with a live token.

template <class T, class token_t = typename std::vector<T>::size_type>
class thick_list_t {
public:
    typedef token_t token_type;
    typedef T value_type;

    // adds a value, returning a token_t (a numeric type) for
    // accessing it.  The expression value != T() _must_ be true.
    token_type add(T& value) {
        assert(value != T());

        if (free_.empty()) {
            token_t ret = values_.size();
            values_.push_back(value);
        } else {
            token_t ret = free_.back();
            assert(values_[ret] != T());
            free_.pop_back();
            values_[ret] = value;
        }
    }

    // Returns a value for the given token, or T() if the token is not
    // alive.
    T& get(token_type token) {
        if (0 <= token && token < values_.size()) {
            return values_[token];
        } else {
            return T();
        }
    }

    // Drops a value for the given token.
    void drop(token_type token) {
        assert(0 <= token && token < values_.size());
        assert(free_.end() == std::find(free_.begin(), free_.end(), token));

        values_[token] = T();
        free_.push_back(token);
    }

private:
    std::vector<T> values_;
    std::vector<token_t> free_;
};



#endif  // __CONTAINERS_THICK_LIST_HPP__
