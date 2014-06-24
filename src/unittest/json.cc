// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "http/json.hpp"
#include "http/json/cJSON.hpp"
#include "http/json/json_adapter.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "stl_utils.hpp"
#include "utils.hpp"


int json_cmp(cJSON *l, cJSON *r);

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

// This has complexity O(n^2) due to its use of `cJSON_slow_GetArrayItem()`.
// Ok for unit tests, but don't use it in production code.
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
                int lsize = cJSON_slow_GetArraySize(l),
                    rsize = cJSON_slow_GetArraySize(r);
                for (int i = 0; i < lsize; ++i) {
                    if (i >= rsize) {
                        return 1;  // e.g. cmp([0, 1], [0])
                    }
                    int cmp = json_cmp(cJSON_slow_GetArrayItem(l, i),
                                       cJSON_slow_GetArrayItem(r, i));
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

int sign(int x) {
    return (0 < x) - (x < 0);
}

#define ASSERT_SAME_SIGN(x, y) ASSERT_EQ(sign(x), sign(y))

int compare_and_delete(cJSON *l, cJSON *r) {
    int res = json_cmp(l, r);
    cJSON_Delete(l);
    cJSON_Delete(r);
    return res;
}

namespace unittest {
TEST(JSON, ArrayInsertDelete) {
    cJSON *array = cJSON_CreateArray();

    //Insert several objects in to the array
    for (int i = 0; i < 10; ++i) {
        cJSON_AddItemToArray(array, cJSON_CreateNumber(i));
        //This line is basically there to segfault if we have corrupted structure.
        free(cJSON_PrintUnformatted(array));
    }

    //Remove objects from the array at random
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(cJSON_slow_GetArraySize(array), 10 - i);
        cJSON_DeleteItemFromArray(array, randint(10 - i));
        //This line is basically there to segfault if we have corrupted structure.
        free(cJSON_PrintUnformatted(array));
    }

    int count = 0;
    for (int i = 0; i < 3; ++i) {
        int change = randint(10);

        if (change < count) {
            for (int j = 0; j < change; ++j) {
                cJSON_DeleteItemFromArray(array, randint(count));
                count--;
                ASSERT_EQ(cJSON_slow_GetArraySize(array), count);
                //This line is basically there to segfault if we have corrupted structure.
                free(cJSON_PrintUnformatted(array));
            }
        } else {
            for (int j = 0; j < change; ++j) {
                cJSON_AddItemToArray(array, cJSON_CreateNumber(j));
                count++;
                ASSERT_EQ(cJSON_slow_GetArraySize(array), count);
                //This line is basically there to segfault if we have corrupted structure.
                free(cJSON_PrintUnformatted(array));
            }
        }
    }

    cJSON_Delete(array);
}

TEST(JSON, ArrayParseThenInsert) {
    /* Make sure that parsed arrays are ready to be appended to. */
    cJSON *array = cJSON_Parse("[1,2,3]");
    ASSERT_EQ(cJSON_slow_GetArraySize(array), 3);

    cJSON_AddItemToArray(array, cJSON_CreateNumber(4));

    ASSERT_EQ(cJSON_slow_GetArraySize(array), 4);
    cJSON_Delete(array);
}

TEST(JSON, ObjectInsertDelete) {
    cJSON *obj = cJSON_CreateObject();
    std::set<std::string> keys;

    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(cJSON_slow_GetArraySize(obj), i);
        std::string key;
        for (;;) {
            key = rand_string(40);
            if (keys.insert(key).second) {
                break;
            }
        }

        cJSON_AddItemToObject(obj, key.c_str(), cJSON_CreateNumber(i));

        //This line is basically there to segfault if we have corrupted structure.
        free(cJSON_PrintUnformatted(obj));
    }

    int count = 5;
    for (std::set<std::string>::iterator it = keys.begin(); it != keys.end(); ++it) {
        ASSERT_EQ(cJSON_slow_GetArraySize(obj), count);
        cJSON_DeleteItemFromObject(obj, it->c_str());
        count--;

        //This line is basically there to segfault if we have corrupted structure.
        free(cJSON_PrintUnformatted(obj));
    }


    cJSON_Delete(obj);
}

TEST(JSON, Compare) {
    //bools
    ASSERT_SAME_SIGN(-1, compare_and_delete(cJSON_CreateBool(false), cJSON_CreateBool(true)));
    ASSERT_SAME_SIGN(0, compare_and_delete(cJSON_CreateBool(false), cJSON_CreateBool(false)));
    ASSERT_SAME_SIGN(1, compare_and_delete(cJSON_CreateBool(true), cJSON_CreateBool(false)));
    ASSERT_SAME_SIGN(0, compare_and_delete(cJSON_CreateBool(true), cJSON_CreateBool(true)));

    //numbers
    for (int i = -2; i < 12; ++i) {
        for (int j = -2; j < 12; ++j) {
            ASSERT_SAME_SIGN(i - j,
                             compare_and_delete(cJSON_CreateNumber(i),
                                                cJSON_CreateNumber(j)));
        }
    }

    //strings
    {
        const char *ss[] = { "", "a", "ab", "abcdefghij", "b", "bc", "abcd",
                             "aB", "Ab", "ABCDEFGHIJ" };
        int n = sizeof(ss) / sizeof(ss[0]);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                ASSERT_SAME_SIGN(strcmp(ss[i], ss[j]),
                                 compare_and_delete(cJSON_CreateString(ss[i]),
                                                    cJSON_CreateString(ss[j])));
            }
        }
    }

    std::vector<std::vector<std::string> > vs = make_vector(
        std::vector<std::string>(),
        make_vector<std::string>("a"),
        make_vector<std::string>("a", "ab"),
        make_vector<std::string>("b", "ab"),
        make_vector<std::string>("b")
    );

    //Arrays
    {
        const size_t n = vs.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                int expected = vs[i] < vs[j] ? -1 : vs[i] == vs[j] ? 0 : 1;

                ASSERT_SAME_SIGN(expected, compare_and_delete(render_as_json(&vs[i]),
                                                              render_as_json(&vs[j])));
            }
        }
    }

    //Maps
    {
        const size_t n = vs.size();
        std::vector<std::map<std::string, std::string> > ms;
        for (size_t i = 0; i < n; ++i) {
            std::map<std::string, std::string> m;
            for (size_t j = 0; j < vs[i].size(); ++j) {
                m.insert(std::make_pair(std::string(1, 'a' + j),
                                        vs[i][j]));
            }
            ms.push_back(std::move(m));
        }

        for (size_t i = 0; i < ms.size(); ++i) {
            for (size_t j = 0; j < ms.size(); ++j) {
                int expected = ms[i] < ms[j] ? -1 : ms[i] == ms[j] ? 0 : 1;

                ASSERT_SAME_SIGN(expected, compare_and_delete(render_as_json(&ms[i]),
                                                              render_as_json(&ms[j])));
            }
        }
    }

    //different types
    std::vector<int> order;
    order.push_back(cJSON_Array);
    order.push_back(cJSON_False);
    order.push_back(cJSON_True);
    order.push_back(cJSON_NULL);
    order.push_back(cJSON_Number);
    order.push_back(cJSON_Object);
    order.push_back(cJSON_String);

    for (size_t i = 0; i < order.size(); ++i) {
        for (size_t j = 0; j < order.size(); ++j) {
            cJSON *left = cJSON_CreateBlank(), *right = cJSON_CreateBlank();
            left->type = order[i];
            right->type = order[j];

            int expected;

            if (i < j) {
                expected = -1;
            } else if (i > j) {
                expected = 1;
            } else {
                expected = 0;
            }

            // If expected == 0, that means they're the same type, which would lead
            // to it comparing some garbage values, so we ignore these.
            if (expected != 0) {
                ASSERT_SAME_SIGN(expected, compare_and_delete(left, right));
            }
        }
    }
}

}  // namespace unittest
