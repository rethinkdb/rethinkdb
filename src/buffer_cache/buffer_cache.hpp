// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_BUFFER_CACHE_HPP_
#define BUFFER_CACHE_BUFFER_CACHE_HPP_

// Predeclarations are in here.
#include "buffer_cache/types.hpp"


// This file should be kept synced up with buffer_cache/types.hpp,
// which defines what buf_lock_t, cache_t, etc, are.

#include "buffer_cache/mirrored/mirrored.hpp"
#if !defined(VALGRIND) && !defined(NDEBUG)
#include "buffer_cache/semantic_checking.hpp"
#endif
#include "containers/counted.hpp"



class counted_buf_lock_t : public buf_lock_t,
                           public single_threaded_countable_t<counted_buf_lock_t> {
public:
    template <class... Args>
    counted_buf_lock_t(Args &&... args) : buf_lock_t(std::forward<Args>(args)...) { }

private:
    DISABLE_COPYING(counted_buf_lock_t);
};

#endif /* BUFFER_CACHE_BUFFER_CACHE_HPP_ */
