#ifndef CLUSTERING_ADMINISTRATION_LOGGER_HPP_
#define CLUSTERING_ADMINISTRATION_LOGGER_HPP_

#include <stdio.h>

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/circular_buffer.hpp>

#include "clustering/resource.hpp"
#include "rpc/mailbox/typed.hpp"
#include "utils.hpp"

class log_file_writer_t : public home_thread_mixin_t {
public:
    log_file_writer_t(const std::string &filename);
    ~log_file_writer_t();

    std::vector<std::string> get_lines();

private:
    friend void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...);
    void install_on_thread(int i);
    void uninstall_on_thread(int i);
    FILE *file;
    boost::circular_buffer<std::string> lines;
};

class log_file_server_t {
public:
    log_file_server_t(mailbox_manager_t *mm, log_file_writer_t *lfw);
    mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> get_business_card();
private:
    void handle_request(const mailbox_addr_t<void(std::vector<std::string>)> &, auto_drainer_t::lock_t);
    mailbox_manager_t *mailbox_manager;
    log_file_writer_t *writer;
    auto_drainer_t drainer;
    mailbox_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> mailbox;
};

std::vector<std::string> fetch_log_file(
    mailbox_manager_t *mailbox_manager,
    const mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> &mbox,
    signal_t *interruptor) THROWS_ONLY(resource_lost_exc_t, interrupted_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_LOGGER_HPP_ */
