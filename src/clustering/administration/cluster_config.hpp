// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_CLUSTER_CONFIG_HPP_
#define CLUSTERING_ADMINISTRATION_CLUSTER_CONFIG_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rpc/semilattice/view.hpp"

class cluster_config_artificial_table_backend_t :
    public artificial_table_backend_t
{
public:
    cluster_config_artificial_table_backend_t(
            boost::shared_ptr< semilattice_readwrite_view_t<
                auth_semilattice_metadata_t> > _sl_view);

    std::string get_primary_key_name();
    bool read_all_primary_keys(
            signal_t *interruptor,
            std::vector<ql::datum_t> *keys_out,
            std::string *error_out);
    bool read_row(
            ql::datum_t primary_key,
            signal_t *interruptor,
            ql::datum_t *row_out,
            std::string *error_out);
    bool write_row(
            ql::datum_t primary_key,
            ql::datum_t new_value,
            signal_t *interruptor,
            std::string *error_out);

private:
    class doc_t {
    public:
        virtual bool read(
                signal_t *interruptor,
                ql::datum_t *row_out,
                std::string *error_out) = 0;
        virtual bool write(
                signal_t *interruptor,
                const ql::datum_t &value,
                std::string *error_out) = 0;
    protected:
        virtual ~doc_t() { }   // make compiler happy
    };

    class auth_doc_t : public doc_t {
    public:
        auth_doc_t(boost::shared_ptr< semilattice_readwrite_view_t<
            auth_semilattice_metadata_t> > _sl_view) : sl_view(_sl_view) { }
        bool read(
                signal_t *interruptor,
                ql::datum_t *row_out,
                std::string *error_out);
        bool write(
                signal_t *interruptor,
                const ql::datum_t &value,
                std::string *error_out);
    private:
        boost::shared_ptr< semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
            sl_view;
    };

    auth_doc_t auth_doc;

    std::map<std::string, doc_t *> docs;
};

#endif /* CLUSTERING_ADMINISTRATION_CLUSTER_CONFIG_HPP_ */

