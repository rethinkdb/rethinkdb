// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "http/json.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "utils.hpp"
#include "http/json/json_adapter.hpp"

using query_language::json_cmp;

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
        for (int i = 0; i < 1000; ++i) {
            cJSON_AddItemToArray(array, cJSON_CreateNumber(i));
            //This line is basically there to segfault if we have corrupted structure.
            free(cJSON_PrintUnformatted(array));
        }

        //Remove objects from the array at random
        for (int i = 0; i < 1000; ++i) {
            ASSERT_EQ(cJSON_GetArraySize(array), 1000 - i);
            cJSON_DeleteItemFromArray(array, randint(1000 - i));
            //This line is basically there to segfault if we have corrupted structure.
            free(cJSON_PrintUnformatted(array));
        }

        int count = 0;
        for (int i = 0; i < 100; ++i) {
            int change = randint(1000);

            if (change < count) {
                for (int j = 0; j < change; ++j) {
                    cJSON_DeleteItemFromArray(array, randint(count));
                    count--;
                    ASSERT_EQ(cJSON_GetArraySize(array), count);
                    //This line is basically there to segfault if we have corrupted structure.
                    free(cJSON_PrintUnformatted(array));
                }
            } else {
                for (int j = 0; j < change; ++j) {
                    cJSON_AddItemToArray(array, cJSON_CreateNumber(j));
                    count++;
                    ASSERT_EQ(cJSON_GetArraySize(array), count);
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
        ASSERT_EQ(cJSON_GetArraySize(array), 3);

        cJSON_AddItemToArray(array, cJSON_CreateNumber(4));

        ASSERT_EQ(cJSON_GetArraySize(array), 4);
        cJSON_Delete(array);
    }

    TEST(JSON, ObjectInsertDelete) {
        cJSON *obj = cJSON_CreateObject();
        std::set<std::string> keys;

        for (int i = 0; i < 10000; ++i) {
            ASSERT_EQ(cJSON_GetArraySize(obj), i);
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

        int count = 10000;
        for (std::set<std::string>::iterator it = keys.begin(); it != keys.end(); ++it) {
            ASSERT_EQ(cJSON_GetArraySize(obj), count);
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
        for (int i = 0; i < 1000; ++i) {
            for (int j = 0; j < 1000; ++j) {
                if (i < j) {
                    ASSERT_SAME_SIGN(-1, compare_and_delete(cJSON_CreateNumber(i), cJSON_CreateNumber(j)));
                } else if (i == j) {
                    ASSERT_SAME_SIGN(0, compare_and_delete(cJSON_CreateNumber(i), cJSON_CreateNumber(j)));
                } else {
                    ASSERT_SAME_SIGN(1, compare_and_delete(cJSON_CreateNumber(i), cJSON_CreateNumber(j)));
                }
            }
        }

        //strings
        for (int i = 0; i < 1000; ++i) {
            std::string s1 = rand_string(randint(i / 50 + 1)), s2 = rand_string(randint(i / 50 + 1));
            ASSERT_SAME_SIGN(strcmp(s1.c_str(), s2.c_str()), compare_and_delete(cJSON_CreateString(s1.c_str()), cJSON_CreateString(s2.c_str())));
        }

        //Arrays
        for (int i = 0; i < 1000; ++i) {
            int left_size = randint(i / 100 + 1), right_size = randint(i / 100 + 1);

            std::vector<std::string> left, right;
            for (int j = 0; j < left_size; ++j) {
                left.push_back(rand_string(i / 50 + 1));
            }

            for (int j = 0; j < right_size; ++j) {
                right.push_back(rand_string(i / 50 + 1));
            }

            int expected;

            if (left < right) {
                expected = -1;
            } else if (right < left) {
                expected = 1;
            } else {
                expected = 0;
            }

            ASSERT_SAME_SIGN(expected, compare_and_delete(render_as_json(&left), render_as_json(&right)));
        }

        //Maps
        for (int i = 0; i < 10000; ++i) {
            int left_size = randint(i / 100 + 1), right_size = randint(i / 100 + 1);

            std::map<std::string, std::string> left, right;
            for (int j = 0; j < left_size; ++j) {
                left[rand_string(4)] = rand_string(i / 50 + 1);
            }

            for (int j = 0; j < right_size; ++j) {
                right[rand_string(4)] = rand_string(i / 50 + 1);
            }

            int expected;

            if (left < right) {
                expected = -1;
            } else if (right < left) {
                expected = 1;
            } else {
                expected = 0;
            }

            ASSERT_SAME_SIGN(expected, compare_and_delete(render_as_json(&left), render_as_json(&right)));
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

                //if expected == 0 that means there the same type which would
                //lead to it comparing some garbage values so we ignore these
                if (expected != 0) {
                    ASSERT_SAME_SIGN(expected, compare_and_delete(left, right));
                }
            }
        }
    }

} //namespace unittest
