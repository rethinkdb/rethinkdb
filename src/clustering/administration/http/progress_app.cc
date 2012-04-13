#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include "clustering/administration/http/progress_app.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/reactor/metadata.hpp"
#include "containers/uuid.hpp"
#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

template <class protocol_t>
class get_backfiller_business_card_t : public boost::static_visitor<boost::optional<backfiller_business_card_t<protocol_t> > > {
public:
    boost::optional<backfiller_business_card_t<protocol_t> > operator()(const typename reactor_business_card_t<protocol_t>::primary_t &primary) const {
        if (primary.replier) {
            return primary.replier->backfiller_bcard;
        } else {
            return boost::optional<backfiller_business_card_t<protocol_t> >();
        }
    }

    boost::optional<backfiller_business_card_t<protocol_t> > operator()(const typename reactor_business_card_t<protocol_t>::primary_when_safe_t &) const {
        return boost::optional<backfiller_business_card_t<protocol_t> >();
    }


    boost::optional<backfiller_business_card_t<protocol_t> > operator()(const typename reactor_business_card_t<protocol_t>::secondary_up_to_date_t &secondary_up_to_date) const {
        return secondary_up_to_date.replier.backfiller_bcard;
    }

    boost::optional<backfiller_business_card_t<protocol_t> > operator()(const typename reactor_business_card_t<protocol_t>::secondary_without_primary_t &secondary_without_primary) const {
        return secondary_without_primary.backfiller;
    }

    boost::optional<backfiller_business_card_t<protocol_t> > operator()(const typename reactor_business_card_t<protocol_t>::secondary_backfilling_t &) const {
        return boost::optional<backfiller_business_card_t<protocol_t> >();
    }

    boost::optional<backfiller_business_card_t<protocol_t> > operator()(const typename reactor_business_card_t<protocol_t>::nothing_when_safe_t &nothing_when_safe) const {
        return nothing_when_safe.backfiller;
    }

    boost::optional<backfiller_business_card_t<protocol_t> > operator()(const typename reactor_business_card_t<protocol_t>::nothing_t &) const {
        return boost::optional<backfiller_business_card_t<protocol_t> >();
    }

    boost::optional<backfiller_business_card_t<protocol_t> > operator()(const typename reactor_business_card_t<protocol_t>::nothing_when_done_erasing_t &) const {
        return boost::optional<backfiller_business_card_t<protocol_t> >();
    }
};

static const char *any_machine_id_wildcard = "_";

progress_app_t::progress_app_t(clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> >& _directory_metadata, mailbox_manager_t *_mbox_manager)
    : directory_metadata(_directory_metadata), mbox_manager(_mbox_manager)
{ }

class request_record_t {
public:
    promise_t<float> *promise;
    signal_timer_t *timer;
    mailbox_t<void(float)> *resp_mbox;

    request_record_t(promise_t<float> *_promise, signal_timer_t *_timer, mailbox_t<void(float)> *_resp_mbox)
        : promise(_promise), timer(_timer), resp_mbox(_resp_mbox)
    { }

    ~request_record_t() {
        delete promise;
        delete timer;
        delete resp_mbox;
    }
};

http_res_t progress_app_t::handle(const http_req_t &req) {
    connectivity_service_t *connectivity_service = directory_metadata->get_directory_service()->get_connectivity_service();
    connectivity_service_t::peers_list_freeze_t freeze_peers(connectivity_service);
    std::set<peer_id_t> peers_list = connectivity_service->get_peers_list();

    http_req_t::resource_t::iterator it = req.resource.begin();

    boost::optional<machine_id_t> requested_machine_id;
    if (it != req.resource.end()) {
        if (*it != any_machine_id_wildcard) {
            requested_machine_id = str_to_uuid(*it);
            if (requested_machine_id->is_nil()) {
                throw schema_mismatch_exc_t(strprintf("Failed to parse %s as valid uuid\n", it->c_str()));
            }
        }
        ++it;
    }

    boost::optional<namespace_id_t> requested_namespace_id;
    if (it != req.resource.end()) {
        if (*it != any_machine_id_wildcard) {
            requested_namespace_id = str_to_uuid(*it);
            if (requested_namespace_id->is_nil()) {
                throw schema_mismatch_exc_t(strprintf("Failed to parse %s as valid uuid\n", it->c_str()));
            }
        }
        ++it;
    }

    typedef boost::ptr_map<memcached_protocol_t::region_t, request_record_t> region_to_request_record_t;
    typedef std::map<reactor_activity_id_t, boost::ptr_map<memcached_protocol_t::region_t, request_record_t> > activity_id_map_t;
    typedef std::map<namespace_id_t, activity_id_map_t> namespace_id_map_t;
    typedef std::map<machine_id_t, namespace_id_map_t> machine_id_map_t;

    machine_id_map_t promise_map;


    for (std::set<peer_id_t>::iterator p_it  = peers_list.begin();
                                       p_it != peers_list.end();
                                       ++p_it) {
        if (!requested_machine_id || requested_machine_id == directory_metadata->get_value(*p_it).get().machine_id) {
            /* Add an entry for the machine. */
            machine_id_t m_id = directory_metadata->get_value(*p_it).get().machine_id;

            typedef std::map<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<memcached_protocol_t> > > reactor_bcard_map_t;
            reactor_bcard_map_t bcard_map = directory_metadata->get_value(*p_it).get().memcached_namespaces.reactor_bcards;

            for (std::map<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<memcached_protocol_t> > >::iterator n_it  = bcard_map.begin();
                                                                                                                               n_it != bcard_map.end();
                                                                                                                               ++n_it) {
                if (!requested_namespace_id || requested_namespace_id == n_it->first) {

                    for (reactor_business_card_t<memcached_protocol_t>::activity_map_t::iterator a_it  = n_it->second.internal.activities.begin();
                                                                                                 a_it != n_it->second.internal.activities.end();
                                                                                                 ++a_it) {
                        reactor_business_card_t<memcached_protocol_t>::primary_when_safe_t *primary_when_safe = 
                            boost::get<reactor_business_card_t<memcached_protocol_t>::primary_when_safe_t>(&(a_it->second.second));

                        if (primary_when_safe) {
                            //cJSON *backfills= cJSON_CreateObject();
                            //cJSON_AddItemToObject(namespace_info, get_string(render_as_json(&a_it->second.first, 0)).c_str(), backfills);

                            for (std::vector<reactor_business_card_details::backfill_location_t>::iterator b_it  = primary_when_safe->backfills_waited_on.begin();
                                                                                                           b_it != primary_when_safe->backfills_waited_on.end();
                                                                                                           ++b_it) {
                                namespaces_directory_metadata_t<memcached_protocol_t> namespaces_directory_metadata = directory_metadata->get_value(b_it->peer_id).get().memcached_namespaces;

                                if (!std_contains(namespaces_directory_metadata.reactor_bcards, n_it->first)) {
                                    debugf("Reactor bcard not found\n");
                                    continue;
                                }

                                if (!std_contains(namespaces_directory_metadata.reactor_bcards[n_it->first].internal.activities, b_it->activity_id)) {
                                    debugf("Activity id not found\n");
                                    continue;
                                }

                                std::pair<memcached_protocol_t::region_t, reactor_business_card_t<memcached_protocol_t>::activity_t> region_activity_pair = 
                                    namespaces_directory_metadata.reactor_bcards[n_it->first].internal.activities[b_it->activity_id];

                                boost::optional<backfiller_business_card_t<memcached_protocol_t> > backfiller = boost::apply_visitor(get_backfiller_business_card_t<memcached_protocol_t>(), region_activity_pair.second);
                                if (backfiller) {
                                    promise_t<float> *value = new promise_t<float>;
                                    mailbox_t<void(float)> *resp_mbox = new mailbox_t<void(float)>(mbox_manager, boost::bind(&promise_t<float>::pulse, value, _1));

                                    send(mbox_manager, backfiller->request_progress_mailbox, b_it->backfill_session_id, resp_mbox->get_address());

                                    //TODO this needs a timeout, and an interruptor
                                    
                                    signal_timer_t *timer = new signal_timer_t(500);

                                    rassert(!std_contains(promise_map[m_id][n_it->first][a_it->first], a_it->second.first));
                                    promise_map[m_id][n_it->first][a_it->first].insert(a_it->second.first, new request_record_t(value, timer, resp_mbox));
                                    /* wait_any_t waiter(&timer, value.get_ready_signal());

                                    waiter.wait();
                                    if (value.get_ready_signal()->is_pulsed()) {
                                        float response = value.wait();
                                        cJSON_AddItemToObject(backfills, get_string(render_as_json(&region_activity_pair.first, 0)).c_str(), cJSON_CreateNumber(response));
                                    } else {
                                        cJSON_AddItemToObject(backfills, get_string(render_as_json(&region_activity_pair.first, 0)).c_str(), cJSON_CreateString("Timeout"));
                                    } */
                                } else {
                                    //cJSON_AddItemToObject(backfills, get_string(render_as_json(&region_activity_pair.first, 0)).c_str(), cJSON_CreateString("backfiller not found"));
                                }
                            }
                        }

                        reactor_business_card_t<memcached_protocol_t>::secondary_backfilling_t *secondary_backfilling = 
                            boost::get<reactor_business_card_t<memcached_protocol_t>::secondary_backfilling_t>(&(a_it->second.second));

                        if (secondary_backfilling) {
                            reactor_business_card_details::backfill_location_t b_loc = secondary_backfilling->backfill;

                            //cJSON *backfills= cJSON_CreateObject();
                            //cJSON_AddItemToObject(namespace_info, get_string(render_as_json(&a_it->second.first, 0)).c_str(), backfills);

                            namespaces_directory_metadata_t<memcached_protocol_t> namespaces_directory_metadata = directory_metadata->get_value(b_loc.peer_id).get().memcached_namespaces;

                            if (!std_contains(namespaces_directory_metadata.reactor_bcards, n_it->first)) {
                                continue;
                            }

                            if (!std_contains(namespaces_directory_metadata.reactor_bcards[n_it->first].internal.activities, b_loc.activity_id)) {
                                continue;
                            }

                            std::pair<memcached_protocol_t::region_t, reactor_business_card_t<memcached_protocol_t>::activity_t> region_activity_pair = 
                                namespaces_directory_metadata.reactor_bcards[n_it->first].internal.activities[b_loc.activity_id];

                            boost::optional<backfiller_business_card_t<memcached_protocol_t> > backfiller = boost::apply_visitor(get_backfiller_business_card_t<memcached_protocol_t>(), region_activity_pair.second);
                            if (backfiller) {
                                promise_t<float> *value = new promise_t<float>;
                                mailbox_t<void(float)> *resp_mbox = new mailbox_t<void(float)>(mbox_manager, boost::bind(&promise_t<float>::pulse, value, _1));

                                send(mbox_manager, backfiller->request_progress_mailbox, b_loc.backfill_session_id, resp_mbox->get_address());

                                signal_timer_t *timer = new signal_timer_t(500);

                                rassert(!std_contains(promise_map[m_id][n_it->first][a_it->first], a_it->second.first));
                                promise_map[m_id][n_it->first][a_it->first].insert(a_it->second.first, new request_record_t(value, timer, resp_mbox));

                                /* wait_any_t waiter(&timer, value.get_ready_signal());

                                waiter.wait();
                                if (value.get_ready_signal()->is_pulsed()) {
                                    float response = value.wait();
                                    cJSON_AddItemToObject(backfills, get_string(render_as_json(&region_activity_pair.first, 0)).c_str(), cJSON_CreateNumber(response));
                                } else {
                                    cJSON_AddItemToObject(backfills, get_string(render_as_json(&region_activity_pair.first, 0)).c_str(), cJSON_CreateString("Timeout"));
                                } */
                            } else {
                                //cJSON_AddItemToObject(backfills, get_string(render_as_json(&region_activity_pair.first, 0)).c_str(), cJSON_CreateString("backfiller not found"));
                            }
                        }
                    }
                }
            }
        }
    }

    scoped_cJSON_t body(cJSON_CreateObject());

    for (machine_id_map_t::iterator m_it  = promise_map.begin();
                                    m_it != promise_map.end();
                                    ++m_it) {
        cJSON *machine_info = cJSON_CreateObject();
        cJSON_AddItemToObject(body.get(), uuid_to_str(m_it->first).c_str(), machine_info);
        for (namespace_id_map_t::iterator n_it  = m_it->second.begin();
                                          n_it != m_it->second.end();
                                          ++n_it) {
            cJSON *namespace_info = cJSON_CreateObject();
            cJSON_AddItemToObject(machine_info, uuid_to_str(n_it->first).c_str(), namespace_info);
            for (activity_id_map_t::iterator a_it  = n_it->second.begin();
                                             a_it != n_it->second.end();
                                             ++a_it) {
                cJSON *activity_info = cJSON_CreateObject();
                cJSON_AddItemToObject(namespace_info, uuid_to_str(a_it->first).c_str(), activity_info);
                for (region_to_request_record_t::iterator r_it  = a_it->second.begin();
                                                          r_it != a_it->second.end();
                                                          ++r_it) {
                    //We need to make a copy because of json not being able to
                    //handle const pointers, life sucks.
                    memcached_protocol_t::region_t r = r_it->first;
                    wait_any_t waiter(r_it->second->timer, r_it->second->promise->get_ready_signal());
                    waiter.wait();
                    if (r_it->second->promise->get_ready_signal()->is_pulsed()) {
                        float response = r_it->second->promise->wait();
                        cJSON_AddItemToObject(activity_info, get_string(render_as_json(&r, 0)).c_str(), cJSON_CreateNumber(response));
                    } else {
                        cJSON_AddItemToObject(activity_info, get_string(render_as_json(&r, 0)).c_str(), cJSON_CreateString("Timeout"));
                    }
                }
            }
        }
    }

    http_res_t res(200);
    res.set_body("application/json", cJSON_print_std_string(body.get()));
    return res;
}
