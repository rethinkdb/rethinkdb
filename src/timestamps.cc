#include "timestamps.hpp"

#include "debug.hpp"

void debug_print(printf_buffer_t *buf, state_timestamp_t ts) {
    buf->appendf("st_t{");
    debug_print(buf, ts.num);
    buf->appendf("}");
}

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(transition_timestamp_t, before);

