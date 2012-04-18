#include "clustering/administration/http/log_app.hpp"

#include "arch/timing.hpp"
#include "clustering/administration/machine_id_to_peer_id.hpp"

log_http_app_t::log_http_app_t(
        mailbox_manager_t *mm,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> > > > &lmv,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &mitt) :
    mailbox_manager(mm),
    log_mailbox_view(lmv),
    machine_id_translation_table(mitt)
    { }

http_res_t log_http_app_t::handle(const http_req_t &req) {
    http_req_t::resource_t::iterator it = req.resource.begin();
    if (it == req.resource.end()) {
        return http_res_t(404);
    }
    std::string machine_id_str = *it;
    it++;
    if (it != req.resource.end()) {
        return http_res_t(404);
    }
    machine_id_t machine_id;
    try {
        machine_id = str_to_uuid(machine_id_str);
    } catch (std::runtime_error) {
        return http_res_t(404);
    }
    peer_id_t peer_id = machine_id_to_peer_id(machine_id, machine_id_translation_table->get());
    if (peer_id.is_nil()) {
        return http_res_t(404);
    }
    std::map<peer_id_t, mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> > log_mailboxes =
        log_mailbox_view->get();
    std::map<peer_id_t, mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> >::iterator jt =
        log_mailboxes.find(peer_id);
    if (jt == log_mailboxes.end()) {
        /* We get here if they disconnect after we look up their peer ID but
        before we look up their log mailbox. */
        return http_res_t(404);
    }
    signal_timer_t timeout(1000);
    std::vector<std::string> log_lines;
    try {
        log_lines = fetch_log_file(mailbox_manager, jt->second, &timeout);
    } catch (interrupted_exc_t) {
        return http_res_t(500);
    } catch (resource_lost_exc_t) {
        return http_res_t(404);
    }
    /* TODO this is N^2 */
    std::string concatenated;
    for (std::vector<std::string>::iterator kt = log_lines.begin(); kt != log_lines.end(); kt++) {
        concatenated += *kt;
    }
    http_res_t res(200);
    res.set_body("text/plain", concatenated);
    return res;
}
