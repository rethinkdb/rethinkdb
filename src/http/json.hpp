// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef HTTP_JSON_HPP_
#define HTTP_JSON_HPP_

#include <string>
#include <set>
#include <utility>

#include "http/json/cJSON.hpp"
#include "containers/archive/archive.hpp"

class http_res_t;

void http_json_res(cJSON *json, http_res_t *result);

//TODO: do we both merge and cJSON_merge?
//Merge two cJSON objects, crashes if there are overlapping keys
cJSON *merge(cJSON *, cJSON *);
cJSON *cJSON_merge(cJSON *lhs, cJSON *rhs);
void project(cJSON *json, std::set<std::string> keys);

std::string cJSON_print_lexicographic(const cJSON *json);
std::string cJSON_print_std_string(cJSON *json) THROWS_NOTHING;
std::string cJSON_print_unformatted_std_string(cJSON *json) THROWS_NOTHING;
const char *cJSON_type_to_string(int type);

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

    /* Returns the number of items in an array (or object). */
    int GetArraySize() const {
        return cJSON_GetArraySize(val);
    }
    /* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
    cJSON* GetArrayItem(int item) const {
        guarantee(item >= 0);
        return cJSON_GetArrayItem(val, item);
    }
    /* Get item "string" from object. Case insensitive. Returns NULL if unsuccessful. */
    cJSON* GetObjectItem(const char *string) const {
        guarantee(string);
        return cJSON_GetObjectItem(val, string);
    }

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
    cJSON* DeepCopy() const {
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

/* Json serialization */
write_message_t &operator<<(write_message_t &msg, const cJSON &cjson);
MUST_USE archive_result_t deserialize(read_stream_t *s, cJSON *cjson);

#endif /* HTTP_JSON_HPP_ */
