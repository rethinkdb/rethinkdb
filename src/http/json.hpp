#ifndef HTTP_JSON_HPP_
#define HTTP_JSON_HPP_

#include <string>
#include <set>

#include "http/json/cJSON.hpp"
#include "containers/archive/archive.hpp"

class http_res_t;

http_res_t http_json_res(cJSON *json);

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
    std::string Print() const THROWS_NOTHING;
    /* Render a cJSON entity to text for transfer/storage without any formatting. */
    std::string PrintUnformatted() const THROWS_NOTHING;

    /* Returns the number of items in an array (or object). */
    int GetArraySize() const {
        return cJSON_GetArraySize(val);
    }
    /* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
    cJSON* GetArrayItem(int item) const {
        rassert(item >= 0);
        cJSON *retval = cJSON_GetArrayItem(val, item);
        rassert(retval);
        return retval;
    }
    /* Get item "string" from object. Case insensitive. */
    cJSON* GetObjectItem(const char *string) const {
        rassert(string);
        cJSON *retval = cJSON_GetObjectItem(val, string);
        rassert(retval);
        return retval;
    }

    /* Append item to the specified array/object. */
    void AddItemToArray(cJSON *item) {
        rassert(item);
        return cJSON_AddItemToArray(val, item);
    }
    void AddItemToObject(const char *string, cJSON *item) {
        rassert(string); rassert(item);
        return cJSON_AddItemToObject(val, string, item);
    }

    /* Remove/Detatch items from Arrays/Objects. */
    cJSON* DetachItemFromArray(int which) {
        rassert(which >= 0);
        return cJSON_DetachItemFromArray(val, which);
    }
    void DeleteItemFromArray(int which) {
        rassert(which >= 0);
        cJSON_DeleteItemFromArray(val, which);
    }
    cJSON* DetachItemFromObject(const char *string) {
        rassert(string);
        cJSON *retval = cJSON_DetachItemFromObject(val, string);
        rassert(retval);
        return retval;
    }
    void DeleteItemFromObject(const char *string) {
        rassert(string);
        cJSON_DeleteItemFromObject(val, string);
    }

    /* Update array items. */
    void ReplaceItemInArray(int which, cJSON *newitem) {
        rassert(which >= 0); rassert(newitem);
        return cJSON_ReplaceItemInArray(val, which, newitem);
    }
    void ReplaceItemInObject(const char *string, cJSON *newitem) {
        rassert(string); rassert(newitem);
        return cJSON_ReplaceItemInObject(val, string, newitem);
    }

    /* Copy function. */
    cJSON* DeepCopy() const {
        cJSON *retval = cJSON_DeepCopy(val);
        rassert(retval);
        return retval;
    }
};

class copyable_cJSON_t {
private:
    cJSON *val;
public:
    explicit copyable_cJSON_t(cJSON *);
    copyable_cJSON_t(const copyable_cJSON_t &);
    ~copyable_cJSON_t();
    cJSON *get() const;

    write_message_t &operator<<(write_message_t &msg);
    MUST_USE archive_result_t deserialize(read_stream_t *s);

    int type() const {
        return val->type;
    }

    /* Render a cJSON entity to text for transfer/storage. */
    std::string Print() const THROWS_NOTHING;
    /* Render a cJSON entity to text for transfer/storage without any formatting. */
    std::string PrintUnformatted() const THROWS_NOTHING;

    /* Returns the number of items in an array (or object). */
    int GetArraySize() const {
        return cJSON_GetArraySize(val);
    }
    /* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
    cJSON* GetArrayItem(int item) const {
        rassert(item >= 0);
        cJSON *retval = cJSON_GetArrayItem(val, item);
        rassert(retval);
        return retval;
    }
    /* Get item "string" from object. Case insensitive. */
    cJSON* GetObjectItem(const char *string) const {
        rassert(string);
        cJSON *retval = cJSON_GetObjectItem(val, string);
        assert(retval);
        return retval;
    }

    /* Append item to the specified array/object. */
    void AddItemToArray(cJSON *item) {
        rassert(item);
        return cJSON_AddItemToArray(val, item);
    }
    void AddItemToObject(const char *string, cJSON *item) {
        rassert(string); rassert(item);
        return cJSON_AddItemToObject(val, string, item);
    }

    /* Remove/Detatch items from Arrays/Objects. */
    cJSON* DetachItemFromArray(int which) {
        rassert(which >= 0);
        cJSON *retval = cJSON_DetachItemFromArray(val, which);
        rassert(retval);
        return retval;
    }
    void DeleteItemFromArray(int which) {
        rassert(which >= 0);
        cJSON_DeleteItemFromArray(val, which);
    }
    cJSON* DetachItemFromObject(const char *string) {
        rassert(string);
        cJSON *retval = cJSON_DetachItemFromObject(val, string);
        rassert(retval);
        return retval;
    }
    void DeleteItemFromObject(const char *string) {
        rassert(string);
        cJSON_DeleteItemFromObject(val, string);
    }

    /* Update array items. */
    void ReplaceItemInArray(int which, cJSON *newitem) {
        rassert(which >= 0); rassert(newitem);
        return cJSON_ReplaceItemInArray(val, which, newitem);
    }
    void ReplaceItemInObject(const char *string, cJSON *newitem) {
        rassert(string); rassert(newitem);
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

std::string cJSON_print_std_string(cJSON *json) THROWS_NOTHING;
std::string cJSON_print_unformatted_std_string(cJSON *json) THROWS_NOTHING;

void project(cJSON *json, std::set<std::string> keys);

//Merge two cJSON objects, crashes if there are overlapping keys
cJSON *merge(cJSON *, cJSON *);

/* Json serialization */
write_message_t &operator<<(write_message_t &msg, const cJSON &cjson);
MUST_USE archive_result_t deserialize(read_stream_t *s, cJSON *cjson);

#endif /* HTTP_JSON_HPP_ */
