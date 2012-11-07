// Copyright 2010-2012 RethinkDB, all rights reserved.
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
        namespace_repo_t<rdb_protocol_t> *_rdb_namespace_repo,
        admin_tracker_t *_admin_tracker,
        http_app_t *reql_app,
        uuid_t _us,
        std::string path)
{
    std::set<std::string> white_list;
    white_list.insert("/cluster.css");
    white_list.insert("/styles.css");
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
    white_list.insert("/js/codemirror/codemirror.css");
    white_list.insert("/js/codemirror/codemirror.js");
    white_list.insert("/js/codemirror/ambiance.css");
    white_list.insert("/js/codemirror/javascript.js");
    white_list.insert("/images/body_bg_tile.png");
    white_list.insert("/images/icon-magnifying_glass.png");
    white_list.insert("/images/status-panel_bg_tile.png");
    white_list.insert("/images/status_panel-icon_1.png");
    white_list.insert("/images/status_panel-icon_1-error.png");
    white_list.insert("/images/status_panel-icon_2.png");
    white_list.insert("/images/status_panel-icon_2-error.png");
    white_list.insert("/images/status_panel-icon_3.png");
    white_list.insert("/images/status_panel-icon_3-error.png");
    white_list.insert("/images/status_panel-icon_4.png");
    white_list.insert("/images/status_panel-icon_4-error.png");
    white_list.insert("/images/resolve_issue-resolved_icon.png");
    white_list.insert("/images/green-light.png");
    white_list.insert("/images/red-light.png");
    white_list.insert("/images/server-icon.png");
    white_list.insert("/images/graph-icon.png");
    white_list.insert("/images/green-light_glow.png");
    white_list.insert("/images/red-light_glow.png");
    white_list.insert("/images/pencil-icon_big.png");
    white_list.insert("/images/clock-icon.png");
    white_list.insert("/images/resolve_issue-danger_icon.png");
    white_list.insert("/images/resolve_issue-message_icon.png");
    white_list.insert("/images/resolve_issue-clock_icon.png");
    white_list.insert("/images/resolve_issue-details_icon.png");
    white_list.insert("/images/live-icon.png");
    white_list.insert("/images/layers-icon.png");
    white_list.insert("/images/grid-icon.png");
    white_list.insert("/images/document-icon.png");
    white_list.insert("/images/list-square-bullet.png");
    white_list.insert("/images/list-vert-dash.png");
    white_list.insert("/images/list-horiz-dash.png");
    white_list.insert("/images/green-light_glow_small.png");
    white_list.insert("/images/red-light_glow_small.png");
    white_list.insert("/images/bars-icon_server-assignments.png");
    white_list.insert("/images/globe-icon_white.png");
    white_list.insert("/images/bars-icon.png");
    white_list.insert("/images/arrow_down.gif");
    white_list.insert("/images/arrow_right.gif");
    white_list.insert("/images/ajax-loader.gif");
    white_list.insert("/fonts/copse-regular-webfont.eot");
    white_list.insert("/fonts/copse-regular-webfont.svg");
    white_list.insert("/fonts/copse-regular-webfont.ttf");
    white_list.insert("/fonts/copse-regular-webfont.woff");
    white_list.insert("/fonts/inconsolata-bold-webfont.eot");
    white_list.insert("/fonts/inconsolata-bold-webfont.svg");
    white_list.insert("/fonts/inconsolata-bold-webfont.ttf");
    white_list.insert("/fonts/inconsolata-bold-webfont.woff");
    white_list.insert("/fonts/inconsolata-regular-webfont.eot");
    white_list.insert("/fonts/inconsolata-regular-webfont.svg");
    white_list.insert("/fonts/inconsolata-regular-webfont.ttf");
    white_list.insert("/fonts/inconsolata-regular-webfont.woff");
    white_list.insert("/fonts/opensans-bolditalic-webfont.eot");
    white_list.insert("/fonts/opensans-bolditalic-webfont.svg");
    white_list.insert("/fonts/opensans-bolditalic-webfont.ttf");
    white_list.insert("/fonts/opensans-bolditalic-webfont.woff");
    white_list.insert("/fonts/opensans-bold-webfont.eot");
    white_list.insert("/fonts/opensans-bold-webfont.svg");
    white_list.insert("/fonts/opensans-bold-webfont.ttf");
    white_list.insert("/fonts/opensans-bold-webfont.woff");
    white_list.insert("/fonts/opensans-extrabolditalic-webfont.eot");
    white_list.insert("/fonts/opensans-extrabolditalic-webfont.svg");
    white_list.insert("/fonts/opensans-extrabolditalic-webfont.ttf");
    white_list.insert("/fonts/opensans-extrabolditalic-webfont.woff");
    white_list.insert("/fonts/opensans-extrabold-webfont.eot");
    white_list.insert("/fonts/opensans-extrabold-webfont.svg");
    white_list.insert("/fonts/opensans-extrabold-webfont.ttf");
    white_list.insert("/fonts/opensans-extrabold-webfont.woff");
    white_list.insert("/fonts/opensans-italic-webfont.eot");
    white_list.insert("/fonts/opensans-italic-webfont.svg");
    white_list.insert("/fonts/opensans-italic-webfont.ttf");
    white_list.insert("/fonts/opensans-italic-webfont.woff");
    white_list.insert("/fonts/opensans-lightitalic-webfont.eot");
    white_list.insert("/fonts/opensans-lightitalic-webfont.svg");
    white_list.insert("/fonts/opensans-lightitalic-webfont.ttf");
    white_list.insert("/fonts/opensans-lightitalic-webfont.woff");
    white_list.insert("/fonts/opensans-light-webfont.eot");
    white_list.insert("/fonts/opensans-light-webfont.svg");
    white_list.insert("/fonts/opensans-light-webfont.ttf");
    white_list.insert("/fonts/opensans-light-webfont.woff");
    white_list.insert("/fonts/opensans-regular-webfont.eot");
    white_list.insert("/fonts/opensans-regular-webfont.svg");
    white_list.insert("/fonts/opensans-regular-webfont.ttf");
    white_list.insert("/fonts/opensans-regular-webfont.woff");
    white_list.insert("/fonts/opensans-semibolditalic-webfont.eot");
    white_list.insert("/fonts/opensans-semibolditalic-webfont.svg");
    white_list.insert("/fonts/opensans-semibolditalic-webfont.ttf");
    white_list.insert("/fonts/opensans-semibolditalic-webfont.woff");
    white_list.insert("/fonts/opensans-semibold-webfont.eot");
    white_list.insert("/fonts/opensans-semibold-webfont.svg");
    white_list.insert("/fonts/opensans-semibold-webfont.ttf");
    white_list.insert("/fonts/opensans-semibold-webfont.woff");
    white_list.insert("/fonts/stylesheet.css");
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
    distribution_app.init(new distribution_app_t(metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, _semilattice_metadata), _namespace_repo,
                                                 metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, _semilattice_metadata), _rdb_namespace_repo));

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
}

administrative_http_server_manager_t::~administrative_http_server_manager_t() {
    /* This must be declared in the `.cc` file because the definitions of the
    destructors for the things in `scoped_ptr_t`s are not available from
    the `.hpp` file. */
}

int administrative_http_server_manager_t::get_port() const {
    return server->get_port();
}
