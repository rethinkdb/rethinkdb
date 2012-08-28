#include "clustering/administration/http/server.hpp"

#include "clustering/administration/http/cyanide.hpp"
#include "clustering/administration/http/directory_app.hpp"
#include "clustering/administration/http/distribution_app.hpp"
#include "clustering/administration/http/issues_app.hpp"
#include "clustering/administration/http/last_seen_app.hpp"
#include "clustering/administration/http/log_app.hpp"
#include "clustering/administration/http/progress_app.hpp"
#include "clustering/administration/http/semilattice_app.hpp"
#include "clustering/administration/http/stat_app.hpp"
#include "clustering/administration/http/combining_app.hpp"
#include "http/file_app.hpp"
#include "http/http.hpp"
#include "http/routing_app.hpp"
#include "rpc/semilattice/view/field.hpp"

std::map<peer_id_t, log_server_business_card_t> get_log_mailbox(const std::map<peer_id_t, cluster_directory_metadata_t> &md) {
    std::map<peer_id_t, log_server_business_card_t> out;
    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator it = md.begin(); it != md.end(); it++) {
        out.insert(std::make_pair(it->first, it->second.log_mailbox));
    }
    return out;
}

std::map<peer_id_t, machine_id_t> get_machine_id(const std::map<peer_id_t, cluster_directory_metadata_t> &md) {
    std::map<peer_id_t, machine_id_t> out;
    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator it = md.begin(); it != md.end(); it++) {
        out.insert(std::make_pair(it->first, it->second.machine_id));
    }
    return out;
}

administrative_http_server_manager_t::administrative_http_server_manager_t(
        int port,
        mailbox_manager_t *mbox_manager,
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata,
        clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > _directory_metadata,
        namespace_repo_t<memcached_protocol_t> *_namespace_repo,
        admin_tracker_t *_admin_tracker,
        local_issue_tracker_t *_local_issue_tracker,
        http_app_t *reql_app,
        uuid_t _us,
        std::string path) :
            bound_issue("PORT_CONFLICT",
                        true,
                        strprintf("could not bind to administrative http server port at %d", port)),
            bound_subscription(&bound_issue_tracker_entry)
{
    std::set<std::string> white_list;
    white_list.insert("/cluster.css");
    white_list.insert("/cluster.html");
    white_list.insert("/cluster-min.js");
    white_list.insert("/favicon.ico");
    white_list.insert("/js/backbone.js");
    white_list.insert("/js/rethinkdb.js");
    white_list.insert("/js/encoding.js");
    white_list.insert("/js/bootstrap/bootstrap-alert.js");
    white_list.insert("/js/bootstrap/bootstrap-modal.js");
    white_list.insert("/js/bootstrap/bootstrap-tab.js");
    white_list.insert("/js/bootstrap/bootstrap-typeahead.js");
    white_list.insert("/js/bootstrap/bootstrap-collapse.js");
    white_list.insert("/js/bootstrap/bootstrap-carousel.js");
    white_list.insert("/js/bootstrap/bootstrap-button.js");
    white_list.insert("/js/bootstrap/bootstrap-dropdown.js");
    white_list.insert("/js/bootstrap/bootstrap-tooltip.js");
    white_list.insert("/js/bootstrap/bootstrap-popover.js");
    white_list.insert("/js/d3.v2.min.js");
    white_list.insert("/js/rdb_cubism.v1.js");
    white_list.insert("/js/date-en-US.js");
    white_list.insert("/js/jquery.cookie.js");
    white_list.insert("/js/flot/jquery.flot.js");
    white_list.insert("/js/flot/jquery.flot.resize.js");
    white_list.insert("/js/handlebars-1.0.0.beta.6.js");
    white_list.insert("/js/jquery-1.7.2.min.js");
    white_list.insert("/js/jquery.color.js");
    white_list.insert("/js/jquery.dataTables.min.js");
    white_list.insert("/js/jquery.form.js");
    white_list.insert("/js/jquery.hotkeys.js");
    white_list.insert("/js/jquery.sparkline.min.js");
    white_list.insert("/js/jquery.timeago.js");
    white_list.insert("/js/jquery.validate.min.js");
    white_list.insert("/js/underscore-min.js");
    white_list.insert("/js/xdate.js");
    white_list.insert("/js/chosen/chosen.jquery.min.js");
    white_list.insert("/js/chosen/chosen.css");
    white_list.insert("/js/chosen/chosen-sprite.png");
    white_list.insert("/js/jquery.ba-outside-events.min.js");
    white_list.insert("/images/ajax-loader.gif");
    white_list.insert("/images/arrow_right.gif");
    white_list.insert("/images/arrow_down.gif");
    white_list.insert("/images/alert-icon_small.png");
    white_list.insert("/images/critical-issue_small.png");
    white_list.insert("/images/information-icon_small.png");
    white_list.insert("/images/right-arrow_16x16.png");
    white_list.insert("/images/right-arrow_12x12.png");
    white_list.insert("/images/down-arrow_16x16.png");
    white_list.insert("/images/down-arrow_12x12.png");
    white_list.insert("/images/glyphicons-halflings.png");
    white_list.insert("/images/glyphicons-halflings-white.png");
    white_list.insert("/images/loading.gif");
    white_list.insert("/images/walkthrough/1.jpg");
    white_list.insert("/images/walkthrough/2.jpg");
    white_list.insert("/images/walkthrough/3.jpg");
    white_list.insert("/images/walkthrough/4.jpg");
    white_list.insert("/images/walkthrough/5.jpg");
    white_list.insert("/index.html");
    file_app.init(new file_http_app_t(white_list, path));

    semilattice_app.init(new semilattice_http_app_t(_metadata_change_handler, _directory_metadata, _us));
    directory_app.init(new directory_http_app_t(_directory_metadata));
    issues_app.init(new issues_http_app_t(&_admin_tracker->issue_aggregator));
    stat_app.init(new stat_http_app_t(mbox_manager, _directory_metadata, _semilattice_metadata));
    last_seen_app.init(new last_seen_http_app_t(&_admin_tracker->last_seen_tracker));
    log_app.init(new log_http_app_t(mbox_manager,
        _directory_metadata->subview(&get_log_mailbox),
        _directory_metadata->subview(&get_machine_id)));
    progress_app.init(new progress_app_t(_directory_metadata, mbox_manager));
    distribution_app.init(new distribution_app_t(metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, _semilattice_metadata), _namespace_repo));

#ifndef NDEBUG
    cyanide_app.init(new cyanide_http_app_t);
#endif

    std::map<std::string, http_app_t *> ajax_routes;
    ajax_routes["directory"] = directory_app.get();
    ajax_routes["issues"] = issues_app.get();
    ajax_routes["stat"] = stat_app.get();
    ajax_routes["last_seen"] = last_seen_app.get();
    ajax_routes["log"] = log_app.get();
    ajax_routes["progress"] = progress_app.get();
    ajax_routes["distribution"] = distribution_app.get();
    ajax_routes["semilattice"] = semilattice_app.get();
    ajax_routes["reql"] = reql_app;
    DEBUG_ONLY_CODE(ajax_routes["cyanide"] = cyanide_app.get());

    std::map<std::string, http_json_app_t *> default_views;
    default_views["semilattice"] = semilattice_app.get();
    default_views["directory"] = directory_app.get();
    default_views["issues"] = issues_app.get();
    default_views["last_seen"] = last_seen_app.get();

    combining_app.init(new combining_http_app_t(default_views));

    ajax_routing_app.init(new routing_http_app_t(combining_app.get(), ajax_routes));

    std::map<std::string, http_app_t *> root_routes;
    root_routes["ajax"] = ajax_routing_app.get();
    root_routing_app.init(new routing_http_app_t(file_app.get(), root_routes));

    server.init(new http_server_t(port, root_routing_app.get()));
    signal_t *is_bound = server->get_bound_signal();

    // Only bother to create the issue if it hasn't already been able to bind to the port
    if (!is_bound->is_pulsed()) {

        // Creating the new entry will insert the issue into the issue tracker
        bound_issue_tracker_entry.init(
                new local_issue_tracker_t::entry_t(_local_issue_tracker, bound_issue));

        // As soon as the tcp listener connects this reset will be triggered and the issue will
        // be removed from the issue tracker
        bound_subscription.reset(is_bound);
    }
}

administrative_http_server_manager_t::~administrative_http_server_manager_t() {
    /* This must be declared in the `.cc` file because the definitions of the
    destructors for the things in `scoped_ptr_t`s are not available from
    the `.hpp` file. */
}
