#include "clustering/administration/persist.hpp"
#include "containers/archive/file_stream.hpp"

namespace metadata_persistence {

std::string metadata_file(const std::string& file_path) {
    return file_path + "/metadata";
}

std::string errno_to_string(int err) {
    char buffer[200];
    char *res = strerror_r(err, buffer, sizeof(buffer));
    return std::string(res);
}

bool check_existence(const std::string& file_path) THROWS_ONLY(file_exc_t) {
    int res = access(file_path.c_str(), F_OK);
    if (res == 0) {
        return true;
    } else if (errno == ENOENT) {
        return false;
    } else {
        throw file_exc_t(errno_to_string(errno));
    }
}

void create(const std::string& file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice) THROWS_ONLY(file_exc_t) {

    int res = mkdir(file_path.c_str(), 0755);
    if (res != 0) {
        throw file_exc_t("Could not create directory: " + errno_to_string(errno));
    }

    blocking_write_file_stream_t file;
    if (!file.init(metadata_file(file_path).c_str())) {
        throw file_exc_t("Could not create file.");
    }

    write_message_t msg;
    msg << machine_id;
    msg << semilattice;

    res = send_write_message(&file, &msg);
    if (res) {
        throw file_exc_t("Could not write to file.");
    }
}

void update(const std::string& file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice) THROWS_ONLY(file_exc_t) {
    blocking_write_file_stream_t file;
    if (!file.init(metadata_file(file_path).c_str())) {
        throw file_exc_t("Could not create file.");
    }
    write_message_t msg;
    msg << machine_id;
    msg << semilattice;
    int res = send_write_message(&file, &msg);
    if (res) {
        throw file_exc_t("Could not write to file.");
    }
}

void read(const std::string& file_path, machine_id_t *machine_id_out, cluster_semilattice_metadata_t *semilattice_out) THROWS_ONLY(file_exc_t) {

    blocking_read_file_stream_t file;
    if (!file.init(metadata_file(file_path).c_str())) {
        throw file_exc_t("Could not open file for read.");
    }

    int res = deserialize(&file, machine_id_out);
    if (!res) { deserialize(&file, semilattice_out); }
    if (res) {
        throw file_exc_t("File contents invalid.");
    }
}

semilattice_watching_persister_t::semilattice_watching_persister_t(
        const std::string &fp,
        machine_id_t mi,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > v,
        local_issue_tracker_t *lit) :
    file_path(fp), machine_id(mi), view(v),
    subs(boost::bind(&semilattice_watching_persister_t::dump, this), v),
    issue_tracker(lit)
{
    dump();
}

void semilattice_watching_persister_t::dump() {
    try {
        update(file_path, machine_id, view->get());
    } catch (file_exc_t e) {
        persistence_issue.reset(new local_issue_tracker_t::entry_t(
            issue_tracker,
            clone_ptr_t<local_issue_t>(new persistence_issue_t(e.what()))
            ));
        /* TODO: Should we set a timer to retry? */
        return;
    }
    persistence_issue.reset();
}

}  // namespace metadata_persistence

