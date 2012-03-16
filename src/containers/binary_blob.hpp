#ifndef CONTAINERS_BINARY_BLOB_HPP_
#define CONTAINERS_BINARY_BLOB_HPP_

#include "rpc/serialize_macros.hpp"

/* Binary blob that represents some unknown POD type */
class binary_blob_t {
public:
    binary_blob_t() { }

    template<class obj_t>
    explicit binary_blob_t(const obj_t &o) : storage(reinterpret_cast<const uint8_t *>(&o), reinterpret_cast<const uint8_t *>(&o + 1)) { }

    binary_blob_t(const uint8_t *data, size_t size) : storage(data, data+size) { }
    template<class InputIterator>
    binary_blob_t(InputIterator begin, InputIterator end) : storage(begin, end) { }

    /* Constructor in static method form so we can use it as a functor */
    template<class obj_t>
    static binary_blob_t make(const obj_t &o) {
        return binary_blob_t(o);
    }

    size_t size() const {
        return storage.size();
    }

    // Unfortunately C++ compiler gets confused by two overloaded functions
    // which differ only in constness of the argument, so they have to be named
    // differently.
    template<class obj_t>
    static obj_t &get_modifiable(binary_blob_t &blob) {
        rassert(blob.size() == sizeof(obj_t));
        return *reinterpret_cast<obj_t *>(blob.data());
    }

    template<class obj_t>
    static const obj_t &get(const binary_blob_t &blob) {
        rassert(blob.size() == sizeof(obj_t));
        return *reinterpret_cast<const obj_t *>(blob.data());
    }

    void *data() {
        return storage.data();
    }

    const void *data() const {
        return storage.data();
    }

private:
    std::vector<uint8_t> storage;
    RDB_MAKE_ME_SERIALIZABLE_1(storage);
};

inline bool operator==(const binary_blob_t &left, const binary_blob_t &right) {
    return left.size() == right.size() && memcmp(left.data(), right.data(), left.size()) == 0;
}

inline bool operator!=(const binary_blob_t &left, const binary_blob_t &right) {
    return !(left == right);
}

#endif  // CONTAINERS_BINARY_BLOB_HPP_
