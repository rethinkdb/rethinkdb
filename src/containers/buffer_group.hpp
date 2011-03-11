#ifndef __CONTAINERS_BUFFER_GROUP_HPP__
#define __CONTAINERS_BUFFER_GROUP_HPP__

#include <stdlib.h>
#include <vector>

#include "utils2.hpp"

class const_buffer_group_t {
public:
    struct buffer_t {
        ssize_t size;
        const void *data;
    };

    const_buffer_group_t() { }

    void add_buffer(size_t s, const void *d) {
        buffer_t b;
        b.size = s;
        b.data = d;
        buffers_.push_back(b);
    }

    size_t num_buffers() const { return buffers_.size(); }

    buffer_t get_buffer(size_t i) const {
        rassert(i < buffers_.size());
        return buffers_[i];
    }

    size_t get_size() const {
        size_t s = 0;
        for (int i = 0; i < (int)buffers_.size(); i++) {
            s += buffers_[i].size;
        }
        return s;
    }

private:
    std::vector<buffer_t> buffers_;
    DISABLE_COPYING(const_buffer_group_t);
};

class buffer_group_t {
public:
    struct buffer_t {
        ssize_t size;
        void *data;
    };

    buffer_group_t() { }

    void add_buffer(size_t s, void *d) { inner_.add_buffer(s, d); }

    size_t num_buffers() const { return inner_.num_buffers(); }

    buffer_t get_buffer(size_t i) const {
        rassert(i < inner_.num_buffers());
        buffer_t ret;
        const_buffer_group_t::buffer_t tmp = inner_.get_buffer(i);
        ret.size = tmp.size;
        ret.data = const_cast<void *>(tmp.data);
        return ret;
    }

    size_t get_size() const { return inner_.get_size(); }

    friend const const_buffer_group_t *const_view(const buffer_group_t *group);

private:
    const_buffer_group_t inner_;
    DISABLE_COPYING(buffer_group_t);
};

inline const const_buffer_group_t *const_view(const buffer_group_t *group) {
    return &group->inner_;
}




#endif  // __CONTAINERS_BUFFER_GROUP_HPP__
