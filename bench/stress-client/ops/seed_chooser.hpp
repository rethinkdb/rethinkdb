// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef __STRESS_CLIENT_OPS_KEY_CHOOSER_HPP__
#define __STRESS_CLIENT_OPS_KEY_CHOOSER_HPP__

#include "ops/seed_key_generator.hpp"

/* Since there are so many different ways to choose which keys to operate on, we have
an abstract class to do it. */

struct seed_chooser_t {
    /* Attempts to generate "nseeds" unique seeds. Puts them in "seeds". Returns the
    number of seeds actually generated.
    
    For an example of why the return value may be less than "nseeds", consider a seed
    chooser that is supposed to only choose seeds that actually exist in the database.
    Suppose that only three keys are in the database, but "nseeds" is ten. The return
    value will be three. */
    virtual int choose_seeds(seed_t *seeds, int nseeds) = 0;

    virtual ~seed_chooser_t() { }
};

#endif /* __STRESS_CLIENT_OPS_KEY_CHOOSER_HPP__ */
