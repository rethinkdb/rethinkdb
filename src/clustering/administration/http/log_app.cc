#include "clustering/administration/http/log_app.hpp"

#include "arch/timing.hpp"
#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "utils.hpp"

template <class ctx_t>
cJSON *render_as_json(log_message_t *message, const ctx_t &) {
    std::string timestamp_buffer = strprintf("%ld.%09ld", message->timestamp.tv_sec, message->timestamp.tv_nsec);
    scoped_cJSON_t json(cJSON_CreateObject());
    json.AddItemToObject("timestamp", cJSON_CreateString(timestamp_buffer.c_str()));
    json.AddItemToObject("uptime", cJSON_CreateNumber(message->uptime.tv_sec + message->uptime.tv_nsec / 1000000000.0));
    json.AddItemToObject("level", cJSON_CreateString(format_log_level(message->level).c_str()));
    json.AddItemToObject("message", cJSON_CreateString(message->message.c_str()));
    return json.release();
}

log_http_app_t::log_http_app_t(
        mailbox_manager_t *mm,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, log_server_business_card_t> > > &lmv,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &mitt) :
    mailbox_manager(mm),
    log_mailbox_view(lmv),
    machine_id_translation_table(mitt)
    { }

static bool scan_timespec(const char *string, struct timespec *out) {
    char dummy;
    int secs, nsecs;
    int res = sscanf(string, "%d.%d%c", &secs, &nsecs, &dummy);
    switch (res) {
    case 1:
        out->tv_sec = secs;
        out->tv_nsec = 0;
        return true;
    case 2:
        out->tv_sec = secs;
        out->tv_nsec = nsecs;
        return true;
    default:
        return false;
    }
}

http_res_t log_http_app_t::handle(const http_req_t &req) {
    if (req.method != GET) {
        return http_res_t(HTTP_METHOD_NOT_ALLOWED);
    }

    http_req_t::resource_t::iterator it = req.resource.begin();
    if (it == req.resource.end()) {
        return http_res_t(HTTP_NOT_FOUND);
    }
    std::string machine_id_str = *it;
    ++it;
    if (it != req.resource.end()) {
        return http_res_t(HTTP_NOT_FOUND);
    }

    std::vector<machine_id_t> machine_ids;
    std::map<peer_id_t, machine_id_t> all_machines = machine_id_translation_table->get();
    if (machine_id_str == "_") {
        for (std::map<peer_id_t, machine_id_t>::iterator it = all_machines.begin(); it != all_machines.end(); ++it) {
            machine_ids.push_back(it->second);
        }
    } else {
        const char *p = machine_id_str.c_str(), *start = p;
        while (true) {
            while (*p && *p != '+') { ++p; }
            try {
                machine_ids.push_back(str_to_uuid(std::string(start, p - start)));
            } catch (std::runtime_error) {
                return http_res_t(HTTP_NOT_FOUND);
            }
            if (!*p) {
                break;
            } else {
                /* Step over the `+` */
                ++p;
            }
        }
    }

    int max_length = 100;
    struct timespec min_timestamp = {0, 0}, max_timestamp = {time(NULL) + 1000, 0};
    if (boost::optional<std::string> max_length_string = req.find_query_param("max_length")) {
        char dummy;
        int res = sscanf(max_length_string.get().c_str(), "%d%c", &max_length, &dummy);
        if (res != 1) {
            return http_res_t(HTTP_BAD_REQUEST);
        }
    }
    if (boost::optional<std::string> min_timestamp_string = req.find_query_param("min_timestamp")) {
        if (!scan_timespec(min_timestamp_string.get().c_str(), &min_timestamp)) {
            return http_res_t(HTTP_BAD_REQUEST);
        }
    }
    if (boost::optional<std::string> max_timestamp_string = req.find_query_param("max_timestamp")) {
        if (!scan_timespec(max_timestamp_string.get().c_str(), &max_timestamp)) {
            return http_res_t(HTTP_BAD_REQUEST);
        }
    }

    std::vector<peer_id_t> peer_ids;
    for (std::vector<machine_id_t>::iterator it = machine_ids.begin(); it != machine_ids.end(); ++it) {
        peer_id_t pid = machine_id_to_peer_id(*it, machine_id_translation_table->get());
        if (pid.is_nil()) {
            return http_res_t(HTTP_NOT_FOUND);
        }
        peer_ids.push_back(pid);
    }

    scoped_cJSON_t map_to_fill(cJSON_CreateObject());

    cond_t non_interruptor;
    pmap(peer_ids.size(), boost::bind(
        &log_http_app_t::fetch_logs, this, _1,
        machine_ids, peer_ids,
        max_length, min_timestamp, max_timestamp,
        map_to_fill.get(),
        &non_interruptor));

    return http_json_res(map_to_fill.get());
}

void log_http_app_t::fetch_logs(int i,
        const std::vector<machine_id_t> &machines, const std::vector<peer_id_t> &peers,
        int max_messages, struct timespec min_timestamp, struct timespec max_timestamp,
        cJSON *map_to_fill,
        signal_t *interruptor) THROWS_NOTHING {
    std::map<peer_id_t, log_server_business_card_t> bcards = log_mailbox_view->get();
    std::string key = uuid_to_str(machines[i]);
    if (bcards.count(peers[i]) == 0) {
        cJSON_AddItemToObject(map_to_fill, key.c_str(), cJSON_CreateString("lost contact with peer while fetching log"));
    }
    try {
        std::vector<log_message_t> messages = fetch_log_file(
            mailbox_manager, bcards[peers[i]],
            max_messages, min_timestamp, max_timestamp,
            interruptor);
        cJSON_AddItemToObject(map_to_fill, key.c_str(), render_as_json(&messages, 0));
    } catch (interrupted_exc_t) {
        /* ignore */
    } catch (std::runtime_error e) {
        cJSON_AddItemToObject(map_to_fill, key.c_str(), cJSON_CreateString(e.what()));
    } catch (resource_lost_exc_t) {
        cJSON_AddItemToObject(map_to_fill, key.c_str(), cJSON_CreateString("lost contact with peer while fetching log"));
    }
}
