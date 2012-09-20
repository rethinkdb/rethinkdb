#ifndef CLUSTERING_ADMINISTRATION_MAIN_JSON_IMPORT_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_JSON_IMPORT_HPP_

#include <string>
#include <vector>

#include "errors.hpp"

class scoped_cJSON_t;

class json_importer_t {
public:
    // Returns false upon EOF.
    virtual bool next_json(scoped_cJSON_t *out) = 0;

    // Returns true if we can't rule out that key as an acceptable primary key.
    virtual bool might_support_primary_key(const std::string& primary_key) = 0;
    virtual ~json_importer_t() { }
};

class csv_to_json_importer_t : public json_importer_t {
public:
    csv_to_json_importer_t(std::string separators, std::string filepath);

    // Returns false upon EOF.
    bool next_json(scoped_cJSON_t *out);

    bool might_support_primary_key(const std::string& primary_key);

private:
    void import_json_from_file(std::string separators, std::string filepath);

    std::vector<std::string> column_names_;
    std::vector<std::vector<std::string> > rows_;

    size_t position_;

    DISABLE_COPYING(csv_to_json_importer_t);
};


#endif  // CLUSTERING_ADMINISTRATION_MAIN_JSON_IMPORT_HPP_
