#include "perfmon/archive.hpp"

#include "containers/archive/stl_types.hpp"

write_message_t &operator<<(write_message_t &msg, const perfmon_result_t &x) {
    perfmon_result_t::perfmon_result_type_t type = x.get_type();
    msg << type;
    switch (type) {
        case perfmon_result_t::type_value:
            msg << *x.get_string();
            break;
        case perfmon_result_t::type_map:
            msg << x.get_map_size();
            for (perfmon_result_t::const_iterator it = x.begin(); it != x.end(); ++it) {
                msg << it->first;
                rassert(it->second != NULL);
                msg << *it->second;
            }
            break;
        default:
            unreachable("unknown prefmon_result_type_t value");
    }

    return msg;
}

archive_result_t deserialize(read_stream_t *s, perfmon_result_t *thing) {
    perfmon_result_t::perfmon_result_type_t type;
    archive_result_t res = deserialize(s, &type);
    if (res) {
        return res;
    }

    thing->reset_type(type);
    switch (type) {
        case perfmon_result_t::type_value:
            res = deserialize(s, thing->get_string());
            if (res) {
                return res;
            }
            break;
        case perfmon_result_t::type_map:
            size_t size;
            res = deserialize(s, &size);
            if (res) {
                return res;
            }

            for (size_t i = 0; i < size; ++i) {
                std::pair<std::string, perfmon_result_t *> p;
                res = deserialize(s, &p.first);
                if (res) {
                    return res;
                }
                p.second = new perfmon_result_t;
                res = deserialize(s, p.second);
                if (res) {
                    delete p.second;
                    return res;
                }

                std::pair<perfmon_result_t::iterator, bool> insert_result = thing->insert(p.first, p.second);
                if (!insert_result.second) {
                    delete p.second;
                    return ARCHIVE_RANGE_ERROR;
                }
            }
            break;
        default:
            unreachable("unknown prefmon_result_type_t value");
    }

    return ARCHIVE_SUCCESS;
}
