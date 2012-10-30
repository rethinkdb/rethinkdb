// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef __STRESS_CLIENT_OPS_FUZZY_MODEL_HPP__
#define __STRESS_CLIENT_OPS_FUZZY_MODEL_HPP__

#include "ops/seed_chooser.hpp"
#include "utils.hpp"

/* fuzzy_model_t is a model that doesn't actually pay attention to which
keys exist and which ones don't. It just chooses seeds randomly, and the seeds
it chooses may or may not exist in the database. Its advantage over non-fuzzy
models is that you can safely use it concurrently from multiple threads. */

struct fuzzy_model_t {

    fuzzy_model_t(int size) : size(size) { }

    struct random_chooser_t : public seed_chooser_t {
        random_chooser_t(fuzzy_model_t *parent, rnd_distr_t distr, int mu) :
            parent(parent), rnd(xrandom_create(distr, mu)) { }
        int choose_seeds(seed_t *seeds, int nseeds) {
            nseeds = std::min(nseeds, parent->size);
            /* Choose consecutive seeds to avoid duplicates. */
            int start = xrandom(rnd, 0, parent->size - 1);
            for (int i = 0; i < nseeds; i++) {
                seeds[i] = start++;
                if (start == parent->size) start = 0;
            }
            return nseeds;
        }
    private:
        fuzzy_model_t *parent;
        rnd_gen_t rnd;
    };

private:
    int size;
};

#endif /* __STRESS_CLIENT_OPS_FUZZY_MODEL_HPP__ */
