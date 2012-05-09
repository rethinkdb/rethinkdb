#include "clustering/administration/persist.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include "arch/runtime/thread_pool.hpp"
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

static void create_blocking(const std::string& file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice, std::string *out) THROWS_NOTHING {
    int res = mkdir(file_path.c_str(), 0755);
    if (res != 0) {
        *out = "Could not create directory: " + errno_to_string(errno);
        return;
    }

    blocking_write_file_stream_t file;
    if (!file.init(metadata_file(file_path).c_str())) {
        *out = "Could not create file.";
        return;
    }

    write_message_t msg;
    msg << machine_id;
    msg << semilattice;

    res = send_write_message(&file, &msg);
    if (res) {
        *out = "Could not write to file.";
        return;
    }

    *out = "";
    return;
}

void create(const std::string& file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice) THROWS_ONLY(file_exc_t) {
    std::string error;
    thread_pool_t::run_in_blocker_pool(
        boost::bind(&create_blocking, file_path, machine_id, semilattice, &error));
    if (error != "") {
        throw file_exc_t(error);
    }
}

static void update_blocking(const std::string& file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice, std::string *out) THROWS_NOTHING {
    blocking_write_file_stream_t file;
    if (!file.init(metadata_file(file_path).c_str())) {
        *out = "Could not create file.";
        return;
    }
    write_message_t msg;
    msg << machine_id;
    msg << semilattice;
    int res = send_write_message(&file, &msg);
    if (res) {
        *out = "Could not write to file.";
        return;
    }
    *out = "";
    return;
}

void update(const std::string& file_path, machine_id_t machine_id, const cluster_semilattice_metadata_t &semilattice) THROWS_ONLY(file_exc_t) {
    std::string error;
    thread_pool_t::run_in_blocker_pool(
        boost::bind(&update_blocking, file_path, machine_id, semilattice, &error));
    if (error != "") {
        throw file_exc_t(error);
    }
}

static void read_blocking(const std::string& file_path, machine_id_t *machine_id_out, cluster_semilattice_metadata_t *semilattice_out, std::string *out) THROWS_NOTHING {
    blocking_read_file_stream_t file;
    if (!file.init(metadata_file(file_path).c_str())) {
        *out = "Could not open file for read.";
        return;
    }

    int res = deserialize(&file, machine_id_out);
    if (!res) { res = deserialize(&file, semilattice_out); }
    if (res) {
        *out = "File contents invalid.";
        return;
    }
    *out = "";
    return;
}

void read(const std::string& file_path, machine_id_t *machine_id_out, cluster_semilattice_metadata_t *semilattice_out) THROWS_ONLY(file_exc_t) {
    std::string error;
    thread_pool_t::run_in_blocker_pool(
        boost::bind(&read_blocking, file_path, machine_id_out, semilattice_out, &error));
    if (error != "") {
        throw file_exc_t(error);
    }
}

semilattice_watching_persister_t::semilattice_watching_persister_t(
        const std::string &fp,
        machine_id_t mi,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > v,
        local_issue_tracker_t *lit) :
    file_path(fp), machine_id(mi), view(v),
    flush_again(new cond_t), issue_tracker(lit),
    subs(boost::bind(&semilattice_watching_persister_t::on_change, this), v)
{
    coro_t::spawn_sometime(boost::bind(&semilattice_watching_persister_t::dump_loop, this, auto_drainer_t::lock_t(&drainer)));
}

void semilattice_watching_persister_t::dump_loop(auto_drainer_t::lock_t keepalive) {
    try {
        for (;;) {
            try {
                update(file_path, machine_id, view->get());
                persistence_issue.reset();
            } catch (file_exc_t e) {
                persistence_issue.reset(new local_issue_tracker_t::entry_t(
                    issue_tracker,
                    clone_ptr_t<local_issue_t>(new persistence_issue_t(e.what()))
                    ));
            }
            {
                wait_any_t c(flush_again.get(), &stop);
                wait_interruptible(&c, keepalive.get_drain_signal());
            }
            if (flush_again->is_pulsed()) {
                flush_again.reset(new cond_t);
            } else {
                break;
            }
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
    stopped.pulse();
}

void semilattice_watching_persister_t::on_change() {
    if (!flush_again->is_pulsed()) {
        flush_again->pulse();
    }
}

}  // namespace metadata_persistence

