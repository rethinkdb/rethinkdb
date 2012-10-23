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

    void report(const std::string &type, std::list<clone_ptr_t<global_issue_t> > *out) const {
        for (std::map<name_string_t, std::set<uuid_t>, case_insensitive_less_t>::const_iterator it =
                by_name.begin(); it != by_name.end(); it++) {
            if (it->second.size() > 1) {
                out->push_back(clone_ptr_t<global_issue_t>(new name_conflict_issue_t(type, it->first.str(), it->second)));
            }
        }
    }

private:
    class case_insensitive_less_t {
    public:
        bool operator()(const name_string_t &a, const name_string_t &b) {
            return strcasecmp(a.str().c_str(), b.str().c_str()) < 0;
        }
    };

    std::map<name_string_t, std::set<uuid_t>, case_insensitive_less_t> by_name;
};

class namespace_map_t {
public:
    namespace_map_t(const std::map<uuid_t, deletable_t<database_semilattice_metadata_t> > &_databases)
        : databases(_databases) { }

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
                            by_name[db_table_name_t(db.get().name.get(), it->second.get().name.get())].insert(it->first);
                        }
                    }
                }
            }
        }
    }

    void report(const std::string &type,
            std::list<clone_ptr_t<global_issue_t> > *out) const {
        for (std::map<db_table_name_t, std::set<uuid_t>, case_insensitive_less_t>::const_iterator it =
                by_name.begin(); it != by_name.end(); it++) {
            if (it->second.size() > 1) {
                // TODO: This is awful, why is name_conflict_issue_t a single type for different kinds of issues?
                const std::string formatted_db_table_name = it->first.db_name.str() + "." + it->first.table_name.str();
                out->push_back(clone_ptr_t<global_issue_t>(new name_conflict_issue_t(type, formatted_db_table_name, it->second)));
            }
        }
    }

private:
    struct db_table_name_t {
        name_string_t db_name;
        name_string_t table_name;
        db_table_name_t(const name_string_t& _db_name, const name_string_t& _table_name)
            : db_name(_db_name), table_name(_table_name) { }
    };

    class case_insensitive_less_t {
    public:
        bool operator()(const db_table_name_t &a, const db_table_name_t &b) {
            int cmp = strcasecmp(a.db_name.c_str(), b.db_name.c_str());
            if (cmp != 0) { return cmp < 0; }
            return strcasecmp(a.table_name.c_str(), b.table_name.c_str()) < 0;
        }
    };

    std::map<db_table_name_t, std::set<uuid_t>, case_insensitive_less_t> by_name;
    std::map<uuid_t, deletable_t<database_semilattice_metadata_t> > databases;
};

name_conflict_issue_tracker_t::name_conflict_issue_tracker_t(boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_view)
    : semilattice_view(_semilattice_view) { }

name_conflict_issue_tracker_t::~name_conflict_issue_tracker_t() { }

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
    machines.report("server", &issues);

    name_map_t databases;
    databases.file_away(metadata.databases.databases);
    databases.report("database", &issues);

    return issues;
}
