#include "clustering/administration/issues/name_conflict.hpp"

class name_map_t {
public:
    template<class object_metadata_t>
    void file_away(const std::map<boost::uuids::uuid, deletable_t<object_metadata_t> > &map) {
        for (typename std::map<boost::uuids::uuid, deletable_t<object_metadata_t> >::const_iterator it = map.begin();
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
        for (std::map<std::string, std::set<boost::uuids::uuid> >::iterator it =
                by_name.begin(); it != by_name.end(); it++) {
            if (it->second.size() > 1) {
                out->push_back(clone_ptr_t<global_issue_t>(
                    new name_conflict_issue_t(type, it->first, it->second)
                    ));
            }
        }
    }

private:
    std::map<std::string, std::set<boost::uuids::uuid> > by_name;
};

std::list<clone_ptr_t<global_issue_t> > name_conflict_issue_tracker_t::get_issues() {
    cluster_semilattice_metadata_t metadata = semilattice_view->get();

    std::list<clone_ptr_t<global_issue_t> > issues;

    name_map_t namespaces;
    namespaces.file_away(metadata.dummy_namespaces.namespaces);
    namespaces.file_away(metadata.memcached_namespaces.namespaces);
    namespaces.report("namespace", &issues);

    name_map_t datacenters;
    datacenters.file_away(metadata.datacenters.datacenters);
    datacenters.report("datacenter", &issues);

    name_map_t machines;
    machines.file_away(metadata.machines.machines);
    machines.report("machine", &issues);

    return issues;
}
