// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CJSON_JSON_HPP_
#define CJSON_JSON_HPP_

#include <string>
#include <set>
#include <utility>

#include "cjson/cJSON.hpp"
#include "containers/archive/archive.hpp"

std::string cJSON_print_lexicographic(const cJSON *json);
std::string cJSON_print_std_string(cJSON *json) THROWS_NOTHING;

class scoped_cJSON_t {
private:
    cJSON *val;

    DISABLE_COPYING(scoped_cJSON_t);

    void swap(scoped_cJSON_t &other) {
        cJSON *tmp = val;
        val = other.val;
        other.val = tmp;
    }

public:
    scoped_cJSON_t() : val(NULL) { }
    explicit scoped_cJSON_t(cJSON *v);

    scoped_cJSON_t(scoped_cJSON_t &&movee) : val(movee.val) {
        movee.val = NULL;
    }

    scoped_cJSON_t &operator=(scoped_cJSON_t &&movee) {
        scoped_cJSON_t tmp(std::move(movee));
        swap(tmp);
        return *this;
    }

    ~scoped_cJSON_t();
    cJSON *get() const;
    cJSON *release();
    void reset(cJSON *v);

    int type() const {
        return val->type;
    }

    /* Render a cJSON entity to text for transfer/storage. */
    std::string Print() const THROWS_NOTHING;
    /* Render a cJSON entity to text for transfer/storage without any formatting. */
    std::string PrintUnformatted() const THROWS_NOTHING;
    /* Render a cJSON entitiy to text for lexicographic sorting.  MUST
       be a number or a string. */
    std::string PrintLexicographic() const THROWS_NOTHING;

    /* Append item to the specified array/object. */
    void AddItemToArray(cJSON *item) {
        guarantee(item);
        return cJSON_AddItemToArray(val, item);
    }
    void AddItemToObject(const char *string, cJSON *item) {
        guarantee(string);
        guarantee(item);
        return cJSON_AddItemToObject(val, string, item);
    }
    void AddItemToObject(const char *string, size_t string_size, cJSON *item) {
        guarantee(string);
        guarantee(item);
        return cJSON_AddItemToObjectN(val, string, string_size, item);
    }

    /* Remove/Detatch items from Arrays/Objects. Returns NULL if unsuccessful. */
    cJSON* DetachItemFromArray(int which) {
        guarantee(which >= 0);
        return cJSON_DetachItemFromArray(val, which);
    }
    void DeleteItemFromArray(int which) {
        guarantee(which >= 0);
        cJSON_DeleteItemFromArray(val, which);
    }
    cJSON* DetachItemFromObject(const char *string) {
        guarantee(string);
        return cJSON_DetachItemFromObject(val, string);
    }
    void DeleteItemFromObject(const char *string) {
        guarantee(string);
        cJSON_DeleteItemFromObject(val, string);
    }

    /* Update array items. */
    void ReplaceItemInArray(int which, cJSON *newitem) {
        guarantee(which >= 0);
        guarantee(newitem);
        return cJSON_ReplaceItemInArray(val, which, newitem);
    }
    void ReplaceItemInObject(const char *string, cJSON *newitem) {
        guarantee(string);
        guarantee(newitem);
        return cJSON_ReplaceItemInObject(val, string, newitem);
    }

    /* Copy function. */
    cJSON *DeepCopy() const {
        cJSON *retval = cJSON_DeepCopy(val);
        guarantee(retval);
        return retval;
    }
};

class json_iterator_t {
public:
    explicit json_iterator_t(cJSON *target);

    cJSON *next();
private:
    cJSON *node;
};

class json_object_iterator_t : public json_iterator_t {
public:
    explicit json_object_iterator_t(cJSON *target);
};

class json_array_iterator_t : public json_iterator_t {
public:
    explicit json_array_iterator_t(cJSON *target);
};


#endif /* CJSON_JSON_HPP_ */

