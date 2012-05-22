#ifndef PERFMON_ARCHIVE_HPP_
#define PERFMON_ARCHIVE_HPP_

#include "perfmon.hpp"
#include "containers/archive/boost_types.hpp"
#include "rpc/serialize_macros.hpp"

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(perfmon_result_t::perfmon_result_type_t, int8_t, perfmon_result_t::type_value, perfmon_result_t::type_map);

write_message_t &operator<<(write_message_t &msg, const perfmon_result_t *&thing);
int deserialize(read_stream_t *s, perfmon_result_t **thing);


inline write_message_t &operator<<(write_message_t &msg, const perfmon_result_t &thing) {
    msg << thing.type;
    msg << thing.value_;
    msg << thing.map_;
    return msg;
}

inline int deserialize(read_stream_t *s, perfmon_result_t *thing) {
    int res = deserialize(s, &thing->type);
    if (res) { return res; }
    res = deserialize(s, &thing->value_);
    if (res) { return res; }
    res = deserialize(s, &thing->map_);
    return res;
}

// TODO:  Sigh.  This is depressing.
inline write_message_t &operator<<(write_message_t &msg, perfmon_result_t *const &thing) {
    msg << (thing ? true : false);
    if (thing) {
        msg << *thing;
    }
    return msg;
}

inline int deserialize(read_stream_t *s, perfmon_result_t **thing) {
    bool exists;
    int res = deserialize(s, &exists);
    if (res) { return res; }
    if (exists) {
        perfmon_result_t *tmp = new perfmon_result_t;
        res = deserialize(s, tmp);
        if (res) {
            delete tmp;
        } else {
            *thing = tmp;
        }
    } else {
        *thing = NULL;
    }

    return res;
}




#endif /* PERFMON_ARCHIVE_HPP_ */
