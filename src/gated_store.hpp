#ifndef __GATED_STORE_HPP__
#define __GATED_STORE_HPP__

#include "store.hpp"
#include "concurrency/gate.hpp"

struct gated_get_store_t : public get_store_t {

    gated_get_store_t(get_store_t *internal);

    get_result_t get(const store_key_t &key);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key,
        rget_bound_mode_t right_mode, const store_key_t &right_key);

    // open_t is a sentry object that opens the gate on its constructor and closes
    // the gate on its destructor
    struct open_t {
        open_t(gated_get_store_t *s) : open(&s->gate) { }
    private:
        gate_t::open_t open;
    };

    // TODO: Make this real
    void set_message(std::string) { }

    // Public because it's useful to be able to create a gated_get_store_t before you create
    // the actual get_store_t. Just don't open the gate until you create the actual get_store_t.
    get_store_t *internal;

private:
    gate_t gate;
};

struct gated_set_store_interface_t : public set_store_interface_t {

    gated_set_store_interface_t(set_store_interface_t *internal);

    mutation_result_t change(const mutation_t&);

    struct open_t {
        open_t(gated_set_store_interface_t *s) : open(&s->gate) { }
    private:
        gate_t::open_t open;
    };

    // TODO: Make this real
    void set_message(std::string) { }

    set_store_interface_t *internal;

private:
    gate_t gate;
};

#endif /* __GATED_STORE_HPP__ */
