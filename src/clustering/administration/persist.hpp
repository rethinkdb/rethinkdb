#ifndef CLUSTERING_ADMINISTRATION_PERSIST_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_HPP_

#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"
#include "clustering/administration/issues/json.hpp"

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

    explicit persistence_issue_t(const std::string &_message) : message(_message) { }

    std::string get_description() const {
        return "There was a problem when trying to persist the metadata to "
            "disk locally: " + message;
    }
    cJSON *get_json_description() const {
        issue_json_t json;
        json.critical = true;
        json.description = "There was a problem when trying to persist the metadata to "
            "disk locally: " + message;
        json.time = get_secs();
        json.type.issue_type = PERSISTENCE_ISSUE;

        return render_as_json(&json, 0);
    }

    persistence_issue_t *clone() const {
        return new persistence_issue_t(message);
    }

    std::string message;

    void rdb_serialize(write_message_t &msg) const {
        msg << int8_t(PERSISTENCE_ISSUE_CODE);
        msg << message;
    }

private:
    DISABLE_COPYING(persistence_issue_t);
};

class semilattice_watching_persister_t {
public:
    semilattice_watching_persister_t(
            const std::string &file_path,
            machine_id_t machine_id,
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > view,
            local_issue_tracker_t *issue_tracker);

    /* `stop_and_flush()` finishes flushing the current value to disk but stops
    responding to future changes. It's usually called right before the
    destructor. */
    void stop_and_flush(signal_t *interruptor) THROWS_NOTHING {
        subs.reset();
        stop.pulse();
        wait_interruptible(&stopped, interruptor);
    }

private:
    void dump_loop(auto_drainer_t::lock_t lock);
    void on_change();
    std::string file_path;
    machine_id_t machine_id;
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > view;

    boost::scoped_ptr<cond_t> flush_again;

    local_issue_tracker_t *issue_tracker;
    boost::scoped_ptr<local_issue_tracker_t::entry_t> persistence_issue;

    cond_t stop, stopped;
    auto_drainer_t drainer;

    semilattice_read_view_t<cluster_semilattice_metadata_t>::subscription_t subs;

    DISABLE_COPYING(semilattice_watching_persister_t);
};

}   /* namespace metadata_persistence */

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_HPP_ */
