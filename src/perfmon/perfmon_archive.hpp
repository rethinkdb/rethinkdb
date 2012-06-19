#ifndef PERFMON_ARCHIVE_HPP_
#define PERFMON_ARCHIVE_HPP_

#include "perfmon/perfmon.hpp"
#include "containers/archive/boost_types.hpp"
#include "rpc/serialize_macros.hpp"

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(perfmon_result_t::perfmon_result_type_t, int8_t, perfmon_result_t::type_value, perfmon_result_t::type_map);

// We have custom-implemented serialization functions because we don't
// want to make a way to support serializing a pointer to
// perfmon_result_t.
inline write_message_t &operator<<(write_message_t &msg, const perfmon_result_t &x) {
    msg << x.type;
    msg << x.value_;
    int64_t size = x.map_.size();
    msg << size;
    for (perfmon_result_t::internal_map_t::const_iterator it = x.map_.begin();
         it != x.map_.end();
         ++it) {
        msg << it->first;
        rassert(it->second != NULL);
        msg << *it->second;
    }

    return msg;
}

inline archive_result_t deserialize(read_stream_t *s, perfmon_result_t *thing) {
    archive_result_t res = deserialize(s, &thing->type);
    if (res) { return res; }
    res = deserialize(s, &thing->value_);
    if (res) { return res; }
    int64_t size;
    res = deserialize(s, &size);
    if (res) { return res; }
    if (size < 0) {
        return ARCHIVE_RANGE_ERROR;
    }

    for (int64_t i = 0; i < size; ++i) {
        std::pair<std::string, perfmon_result_t *> p;
        res = deserialize(s, &p.first);
        if (res) { return res; }
        p.second = new perfmon_result_t;
        res = deserialize(s, p.second);
        if (res) {
            delete p.second;
            return res;
        }

        std::pair<perfmon_result_t::internal_map_t::iterator, bool>
            insert_result = thing->map_.insert(p);
        if (!insert_result.second) {
            delete p.second;
            return ARCHIVE_RANGE_ERROR;
        }
    }

    return ARCHIVE_SUCCESS;
}

#endif /* PERFMON_ARCHIVE_HPP_ */
