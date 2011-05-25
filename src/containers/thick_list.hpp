#ifndef __CONTAINERS_THICK_LIST_HPP__
#define __CONTAINERS_THICK_LIST_HPP__

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
        -- value;
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
            return ret;
        } else {
            token_t ret = free_.back();
            rassert(values_[ret] != T());
            free_.pop_back();
            values_[ret] = value;
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

    // TODO: Consider throwing an exception, instead of returning false.
    bool add(token_type known_token, T const& value) {
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

            if (values_.size() > known_token) {
                free_.erase(p.base() - 1);
                values_[known_token] = value;

                return true;
            } else {
                return false;
            }
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

    class thick_list_iterator {
    public:
        T &operator*() {
            // TODO: Add some sanity checking, to catch concurrent modifications
            rassert(values_iterator_ < parent_->values_.size(), "Tried to dereference end() iterator?");
            rassert(free_.find(static_cast<token_t>(values_iterator_)) == free_.end());
            return parent_->values_[values_iterator_];
        }

        // This is the ++operator operator...
        // Be warned, this has runtime up to O(max(tokens) * log(size of free_))
        thick_list_iterator &operator++() {
            ++values_iterator_;
            spool_forward_till_valid();
            return *this;
        }

        bool operator==(const thick_list_iterator &i) const {
            return i.values_iterator_ == values_iterator_;
        }

        bool operator!=(const thick_list_iterator &i) const {
            return i.values_iterator_ != values_iterator_;
        }

    private:
        friend class thick_list<T, token_t>;

        // Be warned, iterator construction is kind of expensive (O((size of parent->free_) * log(size of parent->free_)) running time and similar memory consumption)
        thick_list_iterator(thick_list<T, token_t> *parent, size_t initial_values_iterator) :
                values_iterator_(initial_values_iterator), parent_(parent) {

            // Convert the free list into a set for faster lookups.
            for(size_t i = 0; i < parent_->free_.size(); ++i) {
                free_.insert(parent_->free_[i]);
            }

            spool_forward_till_valid();
        }

        void spool_forward_till_valid() {
            while (values_iterator_ < parent_->values_.size() && free_.find(static_cast<token_t>(values_iterator_)) != free_.end()) {
                ++values_iterator_;
            }
        }

        size_t values_iterator_; // We use a size_t instead of a high level iterator so we can cast it to token_t
        std::set<token_t> free_;
        thick_list<T, token_t> *parent_;
    };

    // Be warned, thick list iterators are not as cheap as you might expect them to be.
    typedef thick_list_iterator expensive_iterator;
    expensive_iterator begin() {
        return thick_list_iterator(this, 0);
    }
    expensive_iterator end() {
        return thick_list_iterator(this, values_.size());
    }

private:
    DISABLE_COPYING(thick_list);  // Copying is stupid.

    std::vector<T> values_;
    std::vector<token_t> free_;
};



#endif  // __CONTAINERS_THICK_LIST_HPP__
