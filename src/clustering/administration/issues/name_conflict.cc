#include "clustering/administration/issues/name_conflict.hpp"

name_conflict_issue_t::name_conflict_issue_t(
        const std::string &_type,
        const std::string &_contested_name,
        const std::set<uuid_t> &_contestants) :
    type(_type), contested_name(_contested_name), contestants(_contestants) { }

std::string name_conflict_issue_t::get_description() const {
    std::string message = "The following " + type + "s are all named '" + contested_name + "': ";
    for (std::set<uuid_t>::iterator it = contestants.begin(); it != contestants.end(); it++) {
        message += uuid_to_str(*it) + "; ";
    }
    return message;
}

cJSON *name_conflict_issue_t::get_json_description() {
    issue_json_t json;
    json.critical = false;
    json.description = "The following " + type + "s are all named '" + contested_name + "': ";
    for (std::set<uuid_t>::iterator it = contestants.begin(); it != contestants.end(); it++) {
        json.description += uuid_to_str(*it) + "; ";
    }
    json.type = "NAME_CONFLICT_ISSUE";
    json.time = get_secs();

    cJSON *res = render_as_json(&json);

    cJSON_AddItemToObject(res, "contested_type", render_as_json(&type));
    cJSON_AddItemToObject(res, "contested_name", render_as_json(&contested_name));
    cJSON_AddItemToObject(res, "contestants", render_as_json(&contestants));

    return res;
}

name_conflict_issue_t *name_conflict_issue_t::clone() const {
    return new name_conflict_issue_t(type, contested_name, contestants);
}

class name_map_t {
public:
    template<class object_metadata_t>
    void file_away(const std::map<uuid_t, deletable_t<object_metadata_t> > &map) {
        for (typename std::map<uuid_t, deletable_t<object_metadata_t> >::const_iterator it = map.begin();
                it != map.end(); it++) {
            if (!it->second.is_deleted()) {
                if (!it->second.get().name.in_conflict()) {
                    by_name[it->second.get().name.get()].insert(it->first);
                }
            }
        }
    }

    void report(const std::string &type,
            std::list<clone_ptr_t<global_issue_t> > *out) {
        for (std::map<std::string, std::set<uuid_t>, case_insensitive_less_t>::iterator it =
                by_name.begin(); it != by_name.end(); it++) {
            if (it->second.size() > 1) {
                out->push_back(clone_ptr_t<global_issue_t>(
                    new name_conflict_issue_t(type, it->first, it->second)));
            }
        }
    }

private:
    class case_insensitive_less_t : public std::binary_function<std::string, std::string, bool> {
    public:
        bool operator()(const std::string &a, const std::string &b) {
            return strcasecmp(a.c_str(), b.c_str()) < 0;
        }
    };

    std::map<std::string, std::set<uuid_t>, case_insensitive_less_t> by_name;
};

class namespace_map_t {
public:
namespace_map_t(const std::map<uuid_t, deletable_t<database_semilattice_metadata_t> > &_databases)
    : databases(_databases)
{ }
    template<class object_metadata_t>
    void file_away(const std::map<uuid_t, deletable_t<object_metadata_t> > &map) {
        for (typename std::map<uuid_t, deletable_t<object_metadata_t> >::const_iterator it = map.begin();
                it != map.end(); it++) {
            if (!it->second.is_deleted()) {
                if (!it->second.get().name.in_conflict() &&
                    !it->second.get().database.in_conflict()) {
                    if (std_contains(databases, it->second.get().database.get())) {
                        deletable_t<database_semilattice_metadata_t> db = databases[it->second.get().database.get()];
                        if (!db.is_deleted() && !db.get().name.in_conflict()) {
                            by_name[db.get().name.get() + "." + it->second.get().name.get()].insert(it->first);
                        }
                    }
                }
            }
        }
    }

    void report(const std::string &type,
            std::list<clone_ptr_t<global_issue_t> > *out) {
        for (std::map<std::string, std::set<uuid_t>, case_insensitive_less_t>::iterator it =
                by_name.begin(); it != by_name.end(); it++) {
            if (it->second.size() > 1) {
                out->push_back(clone_ptr_t<global_issue_t>(
                    new name_conflict_issue_t(type, it->first, it->second)));
            }
        }
    }

private:
    class case_insensitive_less_t : public std::binary_function<std::string, std::string, bool> {
    public:
        bool operator()(const std::string &a, const std::string &b) {
            return strcasecmp(a.c_str(), b.c_str()) < 0;
        }
    };

    std::map<std::string, std::set<uuid_t>, case_insensitive_less_t> by_name;
    std::map<uuid_t, deletable_t<database_semilattice_metadata_t> > databases;
};

std::list<clone_ptr_t<global_issue_t> > name_conflict_issue_tracker_t::get_issues() {
    cluster_semilattice_metadata_t metadata = semilattice_view->get();

    std::list<clone_ptr_t<global_issue_t> > issues;

    namespace_map_t namespaces(metadata.databases.databases);
    namespaces.file_away(metadata.rdb_namespaces->namespaces);
    namespaces.file_away(metadata.dummy_namespaces->namespaces);
    namespaces.file_away(metadata.memcached_namespaces->namespaces);
    namespaces.report("table", &issues);

    name_map_t datacenters;
    datacenters.file_away(metadata.datacenters.datacenters);
    datacenters.report("datacenter", &issues);

    name_map_t machines;
    machines.file_away(metadata.machines.machines);
    machines.report("machine", &issues);

    name_map_t databases;
    databases.file_away(metadata.databases.databases);
    machines.report("databases", &issues);

    return issues;
}
