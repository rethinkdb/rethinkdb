// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_NAME_COLLISION_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_NAME_COLLISION_HPP_

#include <vector>
#include <set>
#include <string>

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/metadata.hpp"
#include "http/json.hpp"
#include "rpc/semilattice/view.hpp"

// Base issue for all name collisions (server, database, and table)
class name_collision_issue_t : public issue_t {
public:
    name_collision_issue_t(const issue_id_t &_issue_id,
                          const name_string_t &_name);
    bool is_critical() const { return true; }

protected:
    template <typename map_t>
    std::vector<uuid_u> get_ids(const map_t &metadata) const;

    const name_string_t name;
};

// Issue for server name collisions
class server_name_collision_issue_t : public name_collision_issue_t {
public:
    explicit server_name_collision_issue_t(const name_string_t &_name);
    const datum_string_t &get_name() const { return server_name_collision_issue_type; }

private:
    void build_info_and_description(const metadata_t &metadata,
                                    ql::datum_t *info_out,
                                    datum_string_t *desc_out) const;

    static const datum_string_t server_name_collision_issue_type;
    static const issue_id_t base_issue_id;
};

// Issue for database name collisions
class db_name_collision_issue_t : public name_collision_issue_t {
public:
    explicit db_name_collision_issue_t(const name_string_t &_name);
    const datum_string_t &get_name() const { return db_name_collision_issue_type; }

private:
    void build_info_and_description(const metadata_t &metadata,
                                    ql::datum_t *info_out,
                                    datum_string_t *desc_out) const;

    static const datum_string_t db_name_collision_issue_type;
    static const issue_id_t base_issue_id;
};

// Issue for table name collisions
class table_name_collision_issue_t : public name_collision_issue_t {
public:
    table_name_collision_issue_t(const name_string_t &_name,
                                const database_id_t &_db_id);
    const datum_string_t &get_name() const { return table_name_collision_issue_type; }

private:
    void build_info_and_description(const metadata_t &metadata,
                                    ql::datum_t *info_out,
                                    datum_string_t *desc_out) const;

    void build_description(const std::string &db_name,
                           const std::vector<namespace_id_t> &ids,
                           datum_string_t *desc_out) const;

    void build_info(const std::string &db_name,
                    const std::vector<namespace_id_t> &ids,
                    ql::datum_t *info_out) const;

    static const datum_string_t table_name_collision_issue_type;
    static const issue_id_t base_issue_id;
    const database_id_t db_id;
};

class name_collision_issue_tracker_t : public issue_tracker_t {
public:
    name_collision_issue_tracker_t(
        issue_multiplexer_t *_parent,
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
