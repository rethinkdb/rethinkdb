// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_NAME_COLLISION_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_NAME_COLLISION_HPP_

#include <vector>
#include <set>
#include <string>

#include "clustering/administration/issues/issue.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

// Base issue for all name collisions (server, database, and table)
class name_collision_issue_t : public issue_t {
public:
    name_collision_issue_t(const issue_id_t &_issue_id,
                           const name_string_t &_name,
                           const std::vector<uuid_u> &_collided_ids);
    bool is_critical() const { return true; }

protected:
    const name_string_t name;
    const std::vector<uuid_u> collided_ids;
};

// Issue for server name collisions
class server_name_collision_issue_t : public name_collision_issue_t {
public:
    explicit server_name_collision_issue_t(
        const name_string_t &_name,
        const std::vector<server_id_t> &_collided_ids);
    const datum_string_t &get_name() const { return server_name_collision_issue_type; }

private:
    static const datum_string_t server_name_collision_issue_type;
    static const issue_id_t base_issue_id;
    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;
};

// Issue for database name collisions
class db_name_collision_issue_t : public name_collision_issue_t {
public:
    explicit db_name_collision_issue_t(
        const name_string_t &_name,
        const std::vector<server_id_t> &_collided_ids);
    const datum_string_t &get_name() const { return db_name_collision_issue_type; }

private:
    static const datum_string_t db_name_collision_issue_type;
    static const issue_id_t base_issue_id;
    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;
};

// Issue for table name collisions
class table_name_collision_issue_t : public name_collision_issue_t {
public:
    table_name_collision_issue_t(
        const name_string_t &_name,
        const database_id_t &_db_id,
        const std::vector<server_id_t> &_collided_ids);
    const datum_string_t &get_name() const { return table_name_collision_issue_type; }

private:
    static const datum_string_t table_name_collision_issue_type;
    static const issue_id_t base_issue_id;
    const database_id_t db_id;
    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;
};

class name_collision_issue_tracker_t : public issue_tracker_t {
public:
    name_collision_issue_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view);

    ~name_collision_issue_tracker_t();

    std::vector<scoped_ptr_t<issue_t> > get_issues() const;

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
        cluster_sl_view;

    DISABLE_COPYING(name_collision_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_NAME_COLLISION_HPP_ */
