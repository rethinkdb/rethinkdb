#include "perfmon/archive.hpp"

#include "containers/archive/stl_types.hpp"

void serialize(write_message_t *wm, const perfmon_result_t &x) {
    perfmon_result_t::perfmon_result_type_t type = x.get_type();
    serialize(wm, type);
    switch (type) {
    case perfmon_result_t::type_value: {
        serialize(wm, *x.get_string());
    } break;
    case perfmon_result_t::type_map: {
        int64_t size = x.get_map_size();
        serialize(wm, size);
        for (perfmon_result_t::const_iterator it = x.begin(); it != x.end(); ++it) {
            serialize(wm, it->first);
            rassert(it->second != NULL);
            serialize(wm, *it->second);
        }
    } break;
    default:
        unreachable("unknown prefmon_result_type_t value");
    }
}

archive_result_t deserialize(read_stream_t *s, perfmon_result_t *thing) {
    perfmon_result_t::perfmon_result_type_t type;
    archive_result_t res = deserialize(s, &type);
    if (bad(res)) {
        return res;
    }

    thing->reset_type(type);
    switch (type) {
    case perfmon_result_t::type_value: {
        res = deserialize(s, thing->get_string());
        if (bad(res)) {
            return res;
        }
    } break;
    case perfmon_result_t::type_map: {
        int64_t size;
        res = deserialize(s, &size);
        if (bad(res)) {
            return res;
        }

        for (int64_t i = 0; i < size; ++i) {
            std::pair<std::string, perfmon_result_t *> p;
            res = deserialize(s, &p.first);
            if (bad(res)) {
                return res;
            }
            p.second = new perfmon_result_t;
            res = deserialize(s, p.second);
            if (bad(res)) {
                delete p.second;
                return res;
            }

            std::pair<perfmon_result_t::iterator, bool> insert_result = thing->insert(p.first, p.second);
            if (!insert_result.second) {
                delete p.second;
                return archive_result_t::RANGE_ERROR;
            }
        }
    } break;
    default:
        unreachable("unknown prefmon_result_type_t value");
    }

    return archive_result_t::SUCCESS;
}
