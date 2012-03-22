#ifndef __SERVER_GATED_STORE_HPP__
#define __SERVER_GATED_STORE_HPP__

#include "memcached/store.hpp"
#include "concurrency/gate.hpp"

class sequence_group_t;

struct gated_get_store_t : public get_store_t {

    explicit gated_get_store_t(get_store_t *internal);

    get_result_t get(const store_key_t &key, sequence_group_t *seq_group, order_token_t token);
    rget_result_t rget(sequence_group_t *seq_group, rget_bound_mode_t left_mode, const store_key_t &left_key,
                       rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token);

    // open_t is a sentry object that opens the gate on its constructor and closes
    // the gate on its destructor
    struct open_t {
        explicit open_t(gated_get_store_t *s) : open(&s->gate) { }
    private:
        threadsafe_gate_t::open_t open;
    };

    // TODO: Make this real
    void set_message(std::string) { }

    // Public because it's useful to be able to create a gated_get_store_t before you create
    // the actual get_store_t. Just don't open the gate until you create the actual get_store_t.
    get_store_t *internal;

private:
    threadsafe_gate_t gate;
};

struct gated_set_store_interface_t : public set_store_interface_t {

    explicit gated_set_store_interface_t(set_store_interface_t *internal);

    mutation_result_t change(sequence_group_t *seq_group, const mutation_t&, order_token_t);

    struct open_t {
        explicit open_t(gated_set_store_interface_t *s) : open(&s->gate) { }
    private:
        threadsafe_gate_t::open_t open;
    };

    // TODO: Make this real
    void set_message(std::string) { }

    set_store_interface_t *internal;

private:
    threadsafe_gate_t gate;
};

#endif /* __SERVER_GATED_STORE_HPP__ */
