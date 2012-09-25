#include <string.h>

#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "utils.hpp"

write_message_t &operator<<(write_message_t &msg, const boost::shared_ptr<scoped_cJSON_t> &cjson) {
    rassert(NULL != cjson.get() && NULL != cjson->get());
    msg << *cjson->get();
    return msg;
}

MUST_USE archive_result_t deserialize(read_stream_t *s, boost::shared_ptr<scoped_cJSON_t> *cjson) {
    cJSON *data = cJSON_CreateBlank();

    archive_result_t res = deserialize(s, data);
    if (res) { return res; }

    *cjson = boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(data));

    return ARCHIVE_SUCCESS;
}

namespace query_language {

/* In a less ridiculous world, the C++ standard wouldn't have dropped designated
   initializers for arrays, and this namespace wouldn't be necessary. */
namespace cJSON_type_ordering {
struct rank_wrapper {
    std::map<int, int> rank;
    rank_wrapper() {
        rank[cJSON_Array]  = 0;
        rank[cJSON_False]  = 1;
        rank[cJSON_True]   = 2;
        rank[cJSON_NULL]   = 3;
        rank[cJSON_Number] = 4;
        rank[cJSON_Object] = 5;
        rank[cJSON_String] = 6;
    }
};
rank_wrapper wrapper;
int cmp(int t1, int t2) { return wrapper.rank[t1] - wrapper.rank[t2]; }
}

// TODO: Rename this function!  It is not part of the cJSON library,
// so it should not be part of the cJSON namepace.
int cJSON_cmp(cJSON *l, cJSON *r, const backtrace_t &backtrace) {
    if (l->type != r->type) {
        return cJSON_type_ordering::cmp(l->type, r->type);
    }
    switch (l->type) {
        case cJSON_False:
            if (r->type == cJSON_True) {
                return -1;
            } else if (r->type == cJSON_False) {
                return 0;
            }
            break;
        case cJSON_True:
            if (r->type == cJSON_True) {
                return 0;
            } else if (r->type == cJSON_False) {
                return 1;
            }
            break;
        case cJSON_NULL:
            return 0;
            break;
        case cJSON_Number:
            if (l->valuedouble < r->valuedouble) {
                return -1;
            } else if (l->valuedouble > r->valuedouble) {
                return 1;
            } else {
                return 0;   // TODO: Handle NaN?
            }
            break;
        case cJSON_String:
            return strcmp(l->valuestring, r->valuestring);
            break;
        case cJSON_Array:
            {
                int lsize = cJSON_GetArraySize(l),
                    rsize = cJSON_GetArraySize(r);
                for (int i = 0; i < lsize; ++i) {
                    if (i >= rsize) {
                        return 1;  // e.g. cmp([0, 1], [0])
                    }
                    int cmp = cJSON_cmp(cJSON_GetArrayItem(l, i), cJSON_GetArrayItem(r, i), backtrace);
                    if (cmp) {
                        return cmp;
                    }
                }
                if (rsize > lsize) return -1;  // e.g. cmp([0], [0, 1]);
                return 0;
            }
            break;
        case cJSON_Object:
            throw runtime_exc_t("Can't compare objects.", backtrace);
            break;
        default:
            unreachable();
            break;
    }
    unreachable();
}

void require_type(const cJSON *json, int type, const backtrace_t &b) {
    if (json->type != type) {
        throw runtime_exc_t(strprintf("Required type: %s but found %s.",
                                      cJSON_type_to_string(type).c_str(),
                                      cJSON_type_to_string(json->type).c_str()),
                            b);
    }
}

} // namespace query_language
