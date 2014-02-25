// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef SAFE_MALLOC_HPP_
#define SAFE_MALLOC_HPP_

// This function must be called once before switching into a multi-threaded context.
void init_safe_malloc();

#endif // SAFE_MALLOC_HPP_
