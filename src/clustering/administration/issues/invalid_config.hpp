// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_INVALID_CONFIG_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_INVALID_CONFIG_HPP_

#include "clustering/administration/issues/issue.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

/* A `need_primary_issue_t` is created when a table's primary replica for a shard was
permanently removed. A `data_loss_issue_t` is created when all of a table's replicas for
a shard are permanently removed. */

class invalid_config_issue_t : public issue_t {
public:
    invalid_config_issue_t(const issue_id_t &issue_id, const namespace_id_t &table_id);
    bool is_critical() const { return true; }
protected:
    virtual datum_string_t build_description(
        const name_string_t &db_name,
        const name_string_t &table_name) const = 0;
    namespace_id_t table_id;
private:
    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;
};

class need_primary_issue_t : public invalid_config_issue_t {
public:
    explicit need_primary_issue_t(const namespace_id_t &_table_id);
    const datum_string_t &get_name() const { return need_primary_issue_type; }
private:
    static const datum_string_t need_primary_issue_type;
    static const issue_id_t base_issue_id;
    datum_string_t build_description(
        const name_string_t &db_name,
        const name_string_t &table_name) const;
};

class data_loss_issue_t : public invalid_config_issue_t {
public:
    explicit data_loss_issue_t(const namespace_id_t &_table_id);
    const datum_string_t &get_name() const { return data_loss_issue_type; }
private:
    static const datum_string_t data_loss_issue_type;
    static const issue_id_t base_issue_id;
    datum_string_t build_description(
        const name_string_t &db_name,
        const name_string_t &table_name) const;
};

class write_acks_issue_t : public invalid_config_issue_t {
public:
    explicit write_acks_issue_t(const namespace_id_t &_table_id);
    const datum_string_t &get_name() const { return write_acks_issue_type; }
private:
    static const datum_string_t write_acks_issue_type;
    static const issue_id_t base_issue_id;
    datum_string_t build_description(
        const name_string_t &db_name,
        const name_string_t &table_name) const;
};

class invalid_config_issue_tracker_t : public issue_tracker_t {
public:
    explicit invalid_config_issue_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view);

    ~invalid_config_issue_tracker_t();

    std::vector<scoped_ptr_t<issue_t> > get_issues() const;

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
        cluster_sl_view;

    DISABLE_COPYING(invalid_config_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_INVALID_CONFIG_HPP_ */

