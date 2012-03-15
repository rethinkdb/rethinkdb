#ifndef __CLUSTERING_ADMINISTRATION_PERSIST_HPP__
#define __CLUSTERING_ADMINISTRATION_PERSIST_HPP__

#include "errors.hpp"
#include <boost/serialization/export.hpp>

#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

namespace metadata_persistence {

class file_exc_t : public std::exception {
public:
    explicit file_exc_t(const std::string& msg) : m(msg) { }
    ~file_exc_t() throw () { }
    const char *what() const throw () {
        return m.c_str();
    }
private:
    std::string m;
};

bool check_existence(const std::string& file_path) THROWS_ONLY(file_exc_t);

void create(const std::string& file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice) THROWS_ONLY(file_exc_t);
void update(const std::string& file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice) THROWS_ONLY(file_exc_t);
void read(const std::string& file_path, machine_id_t *machine_id_out, cluster_semilattice_metadata_t *semilattice_out) THROWS_ONLY(file_exc_t);

class persistence_issue_t : public local_issue_t {
public:
    persistence_issue_t() { }   // for serialization

    persistence_issue_t(const std::string &_message) : message(_message) { }

    std::string get_description() const {
        return "There was a problem when trying to persist the metadata to "
            "disk locally: " + message;
    }

    persistence_issue_t *clone() const {
        return new persistence_issue_t(message);
    }

    std::string message;

    RDB_MAKE_ME_SERIALIZABLE_2(boost::serialization::base_object<local_issue_t>(*this), message);
};

class semilattice_watching_persister_t {
public:
    semilattice_watching_persister_t(
            const std::string &file_path,
            machine_id_t machine_id,
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > view,
            local_issue_tracker_t *issue_tracker);
private:
    void dump();
    std::string file_path;
    machine_id_t machine_id;
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > view;
    semilattice_read_view_t<cluster_semilattice_metadata_t>::subscription_t subs;

    local_issue_tracker_t *issue_tracker;
    boost::scoped_ptr<local_issue_tracker_t::entry_t> persistence_issue;
};

}   /* namespace metadata_persistence */

#endif /* __CLUSTERING_ADMINISTRATION_PERSIST_HPP__ */
