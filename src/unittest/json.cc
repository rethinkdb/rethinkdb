// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "http/json.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "stl_utils.hpp"
#include "unittest/unittest_utils.hpp"

using query_language::json_cmp;

int sign(int x) {
    return (0 < x) - (x < 0);
}

#define ASSERT_SAME_SIGN(x, y) ASSERT_EQ(sign(x), sign(y))

cJSON *render_as_json(std::vector<std::string> *a) {
    cJSON *array = cJSON_CreateArray();
    for (const std::string &s : *a) {
        cJSON_AddItemToArray(array, cJSON_CreateString(s.c_str()));
    }
    return array;
}

cJSON *render_as_json(std::map<std::string, std::string> *m) {
    cJSON *obj = cJSON_CreateObject();
    for (const auto &pair : *m) {
        cJSON_AddItemToObject(obj,
                              pair.first.c_str(),
                              cJSON_CreateString(pair.second.c_str()));
    }
    return obj;
}

int compare_and_delete(cJSON *l, cJSON *r) {
    int res = json_cmp(l, r);
    cJSON_Delete(l);
    cJSON_Delete(r);
    return res;
}

bool valid_json(const char* str) {
    scoped_cJSON_t res(cJSON_Parse(str));
    return res.get() != NULL;
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

TEST(JSON, Parse) {
    EXPECT_FALSE(valid_json("1a"));
    EXPECT_FALSE(valid_json("[],"));
    EXPECT_FALSE(valid_json("]"));
    EXPECT_FALSE(valid_json("["));
    EXPECT_FALSE(valid_json("a"));
    EXPECT_FALSE(valid_json("1e2e3"));
    EXPECT_FALSE(valid_json(""));
    EXPECT_FALSE(valid_json(" "));
    EXPECT_FALSE(valid_json("\x01[]"));
    EXPECT_FALSE(valid_json("\v[]"));

    EXPECT_TRUE(valid_json("\t\r\n [] \t\r\n"));
}

}  // namespace unittest
