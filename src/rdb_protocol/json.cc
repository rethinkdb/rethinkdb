#include <string.h>

#include "rdb_protocol/json.hpp"
#include "rdb_protocol/exceptions.hpp"

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

// TODO: Rename this function!  It is not part of the cJSON library,
// so it should not be part of the cJSON namepace.
int cJSON_cmp(cJSON *l, cJSON *r, const backtrace_t &backtrace) {
    if (l->type != r->type) {
        return l->type - r->type;
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
            return 1;
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
                return -1;  // e.g. cmp([0], [0, 1]);
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



} // namespace query_language
