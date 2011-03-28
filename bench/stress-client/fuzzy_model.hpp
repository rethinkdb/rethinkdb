#ifndef __FUZZY_MODEL_HPP__
#define __FUZZY_MODEL_HPP__

/* fuzzy_model_t is a seed_chooser_t that doesn't actually pay attention to which
keys exist and which ones don't. It just chooses seeds randomly, and the seeds
it chooses may or may not exist in the database. Its advantage over */

struct fuzzy_model_t : public seed_chooser_t {

    fuzzy_model_t(int size, rnd_distr_t distr, int mu) :
        size(size), rnd(xrandom_create(distr, mu)) { }

    int choose_seeds(seed_t *seeds, int nseeds) {
        nseeds = std::min(nseeds, size);
        /* Choose consecutive seeds to avoid duplicates. */
        int start = xrandom(rnd, 0, size - 1);
        for (int i = 0; i < nseeds; i++) {
            seeds[i] = start++;
        }
        return nseeds;
    }

private:
    int size;
    rnd_gen_t rnd;
};

#endif /* __FUZZY_MODEL_HPP__ */
