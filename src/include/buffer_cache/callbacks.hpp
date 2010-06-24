
#ifndef __BUFFER_CACHE_CALLBACKS_HPP__
#define __BUFFER_CACHE_CALLBACKS_HPP__

template <class config_t>
struct block_available_callback {
public:
    typedef typename config_t::buf_t buf_t;

public:
    virtual ~block_available_callback() {}
    virtual void on_block_available(buf_t *buf) = 0;
};

#endif // __BUFFER_CACHE_CALLBACKS_HPP__

