// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <string.h>
#include <algorithm>

#include "rdb_protocol/rdb_protocol_json.hpp"
#include "utils.hpp"

write_message_t &operator<<(write_message_t &msg, const std::shared_ptr<const scoped_cJSON_t> &cjson) {
    rassert(NULL != cjson.get() && NULL != cjson->get());
    msg << *cjson->get();
    return msg;
}

MUST_USE archive_result_t deserialize(read_stream_t *s, std::shared_ptr<const scoped_cJSON_t> *cjson) {
    cJSON *data = cJSON_CreateBlank();

    archive_result_t res = deserialize(s, data);
    if (res) { return res; }

    *cjson = std::shared_ptr<const scoped_cJSON_t>(new scoped_cJSON_t(data));

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

class char_star_cmp_functor {
public:
    bool operator()(const char *x, const char *y) const {
        return strcmp(x, y) < 0;
    }
};

class compare_functor {
public:
    bool operator()(const std::pair<char *, cJSON *> &l, const std::pair<char *, cJSON *> &r) {
        int key_compare = strcmp(l.first, r.first);
        if (key_compare != 0) {
            return key_compare < 0;
        } else {
            return json_cmp(l.second, r.second) < 0;
        }
    }
};

int json_cmp(cJSON *l, cJSON *r) {
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
                    int cmp = json_cmp(cJSON_GetArrayItem(l, i), cJSON_GetArrayItem(r, i));
                    if (cmp != 0) {
                        return cmp;
                    }
                }
                if (rsize > lsize) return -1;  // e.g. cmp([0], [0, 1]);
                return 0;
            }
            break;
        case cJSON_Object:
            {
                std::map<char *, cJSON *, char_star_cmp_functor> lvalues, rvalues;
                json_iterator_t l_json_it(l);

                while (cJSON *cur = l_json_it.next()) {
                    // If this guarantee trips we would have segfaulted on insertion
                    guarantee(cur->string != NULL, "cJSON object is in a map but has no key set... something has gone wrong with cJSON.");
                    // guarantee makes sure there are no duplicates
                    guarantee(lvalues.insert(std::make_pair(cur->string, cur)).second);
                }

                json_iterator_t r_json_it(r);
                while (cJSON *cur = r_json_it.next()) {
                    // If this guarantee trips we would have segfaulted on insertion
                    guarantee(cur->string != NULL, "cJSON object is in a map but has no key set... something has gone wrong with cJSON.");
                    // guarantee makes sure there are no duplicates
                    guarantee(rvalues.insert(std::make_pair(cur->string, cur)).second);
                }

                if (lexicographical_compare(lvalues.begin(), lvalues.end(), rvalues.begin(), rvalues.end(), compare_functor())) {
                    return -1;
                } else if (lexicographical_compare(rvalues.begin(), rvalues.end(), lvalues.begin(), lvalues.end(), compare_functor())) {
                    return 1;
                } else {
                    return 0;
                }
            }

            break;
        default:
            unreachable();
            break;
    }
    unreachable();
}

} // namespace query_language
