#ifndef CONCURRENCY_QUEUE_SINGLE_VALUE_PRODUCER_HPP_
#define CONCURRENCY_QUEUE_SINGLE_VALUE_PRODUCER_HPP_

#include "concurrency/queue/passive_producer.hpp"

template<class T>
class single_value_producer_t : public passive_producer_t<T> {
public:
    single_value_producer_t()
        : passive_producer_t<T>(&availability_control)
    { }

    void give_value(const T &_t) {
        t = _t;
        availability_control.set_available(true);
    }

private:
    T produce_next_value() {
        availability_control.set_available(false);
        return t;
    }

    T t;
    availability_control_t availability_control;
};

#endif
