#ifndef HTTP_JSON_HPP_
#define HTTP_JSON_HPP_

#include <string>
#include <set>

#include "errors.hpp"
#include "http/json/cJSON.hpp"

class scoped_cJSON_t {
private:
    cJSON *val;

public:
    explicit scoped_cJSON_t(cJSON *);
    ~scoped_cJSON_t();
    cJSON *get() const;
    cJSON *release();
    void reset(cJSON *);

    int type() const {
        return val->type;
    }

    /* Render a cJSON entity to text for transfer/storage. */
    std::string Print() const;
    /* Render a cJSON entity to text for transfer/storage without any formatting. */
    std::string PrintUnformatted() const;

    /* Returns the number of items in an array (or object). */
    int GetArraySize() const {
        return cJSON_GetArraySize(val);
    }
    /* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
    cJSON* GetArrayItem(int item) const {
        return cJSON_GetArrayItem(val, item);
    }
    /* Get item "string" from object. Case insensitive. */
    cJSON* GetObjectItem(const char *string) const {
        return cJSON_GetObjectItem(val, string);
    }

    /* Append item to the specified array/object. */
    void AddItemToArray(cJSON *item) {
        return cJSON_AddItemToArray(val, item);
    }
    void AddItemToObject(const char *string, cJSON *item) {
        return cJSON_AddItemToObject(val, string, item);
    }

    /* Remove/Detatch items from Arrays/Objects. */
    cJSON* DetachItemFromArray(int which) {
        return cJSON_DetachItemFromArray(val, which);
    }
    void DeleteItemFromArray(int which) {
        cJSON_DeleteItemFromArray(val, which);
    }
    cJSON* DetachItemFromObject(const char *string) {
        return cJSON_DetachItemFromObject(val, string);
    }
    void DeleteItemFromObject(const char *string) {
        cJSON_DeleteItemFromObject(val, string);
    }

    /* Update array items. */
    void ReplaceItemInArray(int which, cJSON *newitem) {
        return cJSON_ReplaceItemInArray(val, which, newitem);
    }
    void ReplaceItemInObject(const char *string, cJSON *newitem) {
        return cJSON_ReplaceItemInObject(val, string, newitem);
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

std::string cJSON_print_std_string(cJSON *json);
std::string cJSON_print_unformatted_std_string(cJSON *json);

void project(cJSON *json, std::set<std::string> keys);

//Merge two cJSON objects, crashes if there are overlapping keys
cJSON *merge(cJSON *, cJSON *);

#endif /* HTTP_JSON_HPP_ */
