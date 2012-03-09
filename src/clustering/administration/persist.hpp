#ifndef __CLUSTERING_ADMINISTRATION_PERSIST_HPP__
#define __CLUSTERING_ADMINISTRATION_PERSIST_HPP__

#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

namespace metadata_persistence {

class file_exc_t : public std::exception {
public:
    file_exc_t(std::string msg) : m(msg) { }
    ~file_exc_t() throw () { }
    const char *what() const throw () {
        return m.c_str();
    }
private:
    std::string m;
};

bool check_existence(std::string file_path) THROWS_ONLY(file_exc_t);

void create(std::string file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice) THROWS_ONLY(file_exc_t);
void update(std::string file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice) THROWS_ONLY(file_exc_t);
void read(std::string file_path, machine_id_t *machine_id_out, cluster_semilattice_metadata_t *semilattice_out) THROWS_ONLY(file_exc_t);

class semilattice_watching_persister_t {
public:
    semilattice_watching_persister_t(std::string file_path, machine_id_t machine_id, boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > view);
private:
    void dump();
    std::string file_path;
    machine_id_t machine_id;
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > view;
    semilattice_read_view_t<cluster_semilattice_metadata_t>::subscription_t subs;
};

}   /* namespace metadata_persistence */

#endif /* __CLUSTERING_ADMINISTRATION_PERSIST_HPP__ */
