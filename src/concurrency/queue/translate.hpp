#ifndef __CONCURRENCY_QUEUE_TRANSLATE_HPP__
#define __CONCURRENCY_QUEUE_TRANSLATE_HPP__

#include "concurrency/queue/passive_producer.hpp"

/* `casting_passive_producer_t` is useful when you have a
`passive_producer_t<X>` but you need a `passive_producer_t<Y>`, where `X` can
be cast to `Y`. */

template<class input_t, class output_t>
struct casting_passive_producer_t : public passive_producer_t<output_t> {

    casting_passive_producer_t(passive_producer_t<input_t> *source) :
        passive_producer_t<output_t>(source->available), source(source) { }

    output_t produce_next_value() {
        /* Implicit cast from `input_t` to `output_t` in return */
        return source->pop();
    }

private:
    passive_producer_t<input_t> *source;
};

#endif /* __CONCURRENCY_QUEUE_TRANSLATE_HPP__ */
