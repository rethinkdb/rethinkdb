#ifndef __STRESS_CLIENT_OPS_CONSECUTIVE_SEED_MODEL_HPP__
#define __STRESS_CLIENT_OPS_CONSECUTIVE_SEED_MODEL_HPP__

#include "ops/seed_key_generator.hpp"
#include "ops/watcher_and_tracker.hpp"
#include "ops/seed_chooser.hpp"

/* The consecutive-seed model keeps track of the keys in the database by making sure
that the keys that exist occupy a consecutive range of seeds. To ensure this, if you
use insert_op_t or delete_op_t with consecutive_seed_model_t you must make a
consecutive_seed_model_t::insert_chooser_t or consecutive_seed_model_t::delete_chooser_t
to use as their seed_chooser_t. */

struct consecutive_seed_model_t : public existence_watcher_t, public existence_tracker_t {

    consecutive_seed_model_t() : min_seed(0), max_seed(0) { }

public:

    /* min_seed is the seed of the smallest-seeded key in the database. max_seed-1 is
    the seed of the largest-seeded key in the database. If min_seed == max_seed, there
    are no keys in the database.

    min_seed and max_seed can be saved to a file and then restored. */

    seed_t min_seed, max_seed;

public:

    /* existence_tracker_t interface */

    bool key_exists(seed_t seed) {
        return min_seed <= seed && seed < max_seed;
    }

    int key_count() {
        return max_seed - min_seed;
    }

public:

    /* existence_watcher_t interface */

    void on_key_create_destroy(seed_t seed, bool exists) {

        /* Update the seed-range. Of course, keys can only be added and removed at the ends. We
        actually say that keys can only be removed from the beginning and can only be added to
        the end. */
        if (exists != key_exists(seed) && seed != min_seed && seed != max_seed) {
            fprintf(stderr, "Consecutive-seed model is stupid and can't handle what you're trying "
                "to do. If you use delete_op_t or insert_op_t with consecutive_seed_model_t you "
                "have to use consecutive_seed_model_t::insert_chooser_t or consecutive_seed_model"
                "_t::delete_chooser_t as the seed chooser.\n");
            exit(-1);
        }
        if (seed == min_seed && !exists) min_seed++;
        if (seed == max_seed && exists) max_seed++;
    }

public:

    /* If you want to use consecutive_seed_model_t with insert_op_t, use
    insert_chooser_t as the seed-chooser for the insert op. */

    struct insert_chooser_t : public seed_chooser_t {
        insert_chooser_t(consecutive_seed_model_t *p) : parent(p) { }

        int choose_seeds(seed_t *seeds, int nseeds) {
            /* Generate the seeds that are immediately to the right of the live key range */
            for (int i = 0; i < nseeds; i++) {
                /* We DO NOT change parent->max_seed; that's the job of on_key_create_delete(). */
                seeds[i] = parent->max_seed + i;
            }
            return nseeds;
        }
    private:
        consecutive_seed_model_t *parent;
    };

    /* If you want to use consecutive_seed_model_t with delete_op_t, use
    delete_chooser_t as the seed-chooser. */

    struct delete_chooser_t : public seed_chooser_t {
        delete_chooser_t(consecutive_seed_model_t *p) : parent(p) { }

        int choose_seeds(seed_t *seeds, int nseeds) {
            nseeds = std::min(nseeds, (int)(parent->max_seed - parent->min_seed));
            /* Generate the seeds that are at the left end of the live key range */
            for (int i = 0; i < nseeds; i++) {
                /* We DO NOT change parent->min_seed; that's the job of on_key_create_delete(). */
                seeds[i] = parent->min_seed + i;
            }
            return nseeds;
        }
    private:
        consecutive_seed_model_t *parent;
    };

    /* live_chooser_t picks seeds that exist in the database. You can specify
    a random distribution in order to make some seeds "hotter" than others. Do not
    delete the seeds that live_chooser_t returns, because they might be in the
    middle of the seed range. */

    struct live_chooser_t : public seed_chooser_t {
        live_chooser_t(consecutive_seed_model_t *p, rnd_distr_t distr, int mu) :
            parent(p), rnd(xrandom_create(distr, mu)) { }

        int choose_seeds(seed_t *seeds, int nseeds) {

            nseeds = std::min(nseeds, (int)(parent->max_seed - parent->min_seed));

            /* We group the seeds into 16 "classes"; each seed's class is the seed modulo
            16. We use the user-specified random distribution to pick a class and then we
            use a uniform distribution to pick within that class.

            We used to just use the user-specified distribution to pick seeds, but because
            we picked relative to min_seed and max_seed, which seeds were "hot" would change
            as keys were inserted and removed. */

            /* Pick an initial class */
            int class_id = xrandom(rnd, 0, num_classes - 1);

            /* Pick seeds from that class; if we run out, move on to the next class. We
            will never exhaust all the classes because we previously limited "nseeds"
            to the total number of seeds in the database. */
            int seeds_found = 0;
            while (seeds_found < nseeds) {
                seeds_found += choose_seeds_from_class(class_id, &seeds[seeds_found], nseeds - seeds_found);
                class_id = (class_id + 1) % num_classes;
            }

            return nseeds;
        }
    private:
        static const int num_classes = 16;
        int choose_seeds_from_class(int seed_class, seed_t *seeds, int nseeds) {

            /* Find the smallest live seed in the class, and the seed in the class
            immediately past the largest live seed. */
            seed_t min_in_class = parent->min_seed;
            while (min_in_class % num_classes != seed_class) min_in_class++;
            seed_t max_in_class = parent->max_seed;
            while (max_in_class % num_classes != seed_class) max_in_class++;

            int class_size = (max_in_class - min_in_class) / num_classes;
            nseeds = std::min(nseeds, class_size);
            if (nseeds == 0) return 0;

            /* Pick a random starting seed in the class */
            seed_t seed = min_in_class + xrandom(0, class_size - 1) * num_classes;

            /* Fill in with seeds in the class */
            for (int i = 0; i < nseeds; i++) {
                seeds[i] = seed;
                seed += num_classes;
                if (seed == max_in_class) seed = min_in_class;
            }

            return nseeds;
        }
        consecutive_seed_model_t *parent;
        rnd_gen_t rnd;
    };
};

#endif /* __STRESS_CLIENT_OPS_CONSECUTIVE_SEED_MODEL_HPP__ */
