#include "clustering/administration/issues/local.hpp"

#include "utils.hpp"
#include "clustering/administration/persist.hpp"


local_issue_t::local_issue_t(const std::string& _type, bool _critical, const std::string& _description)
        : type(_type), critical(_critical), description(_description) { }

std::string local_issue_t::get_description() const {
    return type + ": " + description;
}

cJSON *local_issue_t::get_json_description() const {
    issue_json_t json;
    json.critical = critical;
    json.description = description;
    json.time = timestamp;
    json.type = type;

    return render_as_json(&json, 0);
}

local_issue_t *local_issue_t::clone() const {
    local_issue_t *ret = new local_issue_t(type, critical, description);
    ret->timestamp = timestamp;
    return ret;
}
