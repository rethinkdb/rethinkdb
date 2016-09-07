// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_GRANT_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_GRANT_HPP

#include "rdb_protocol/table_common.hpp"

namespace auth {

template <typename T>
bool grant(
        boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t>>
            auth_semilattice_view,
        rdb_context_t *rdb_context,
        auth::user_context_t const &user_context,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        T permission_selector_function,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (!user_context.is_admin_user()) {
        *error_out = admin_err_t{
            "Only administrators can grant permissions.",
            query_state_t::FAILED};
        return false;
    }

    if (username.is_admin()) {
        *error_out = admin_err_t{
            "The permissions of the user `" + username.to_string() +
                "` can't be modified.",
            query_state_t::FAILED};
        return false;
    }

    counted_t<const ql::db_t> db;
    if (!rdb_context->cluster_interface->db_find(
            name_string_t::guarantee_valid("rethinkdb"),
            interruptor,
            &db,
            error_out)) {
        return false;
    }

    counted_t<base_table_t> table;
    if (!rdb_context->cluster_interface->table_find(
            name_string_t::guarantee_valid("permissions"),
            db,
            admin_identifier_format_t::uuid,
            interruptor,
            &table,
            error_out)) {
        return false;
    }

    user_context.require_write_permission(rdb_context, db->id, table->get_id());

    auth_semilattice_metadata_t auth_metadata = auth_semilattice_view->get();
    auto grantee = auth_metadata.m_users.find(username);
    if (grantee == auth_metadata.m_users.end() ||
            !static_cast<bool>(grantee->second.get_ref())) {
        *error_out = admin_err_t{
            "User `" + username.to_string() + "` not found.", query_state_t::FAILED};
        return false;
    }

    ql::datum_t old_permissions;
    ql::datum_t new_permissions;
    try {
        grantee->second.apply_write([&](boost::optional<auth::user_t> *user) {
            auto &permissions_ref = permission_selector_function(user->get());
            old_permissions = permissions_ref.to_datum();
            permissions_ref.merge(permissions);
            new_permissions = permissions_ref.to_datum();
        });
    } catch (admin_op_exc_t const &admin_op_exc) {
        *error_out = admin_op_exc.to_admin_err();
        return false;
    }

    auth_semilattice_view->join(auth_metadata);

    // Wait for the metadata to propegate
    rdb_context->get_auth_watchable()->run_until_satisfied(
        [&](auth_semilattice_metadata_t const &metadata) -> bool {
            auth_semilattice_metadata_t copy = metadata;
            semilattice_join(&copy, auth_metadata);
            return copy == metadata;
        },
        interruptor);
    (void) rdb_context;
    (void) interruptor;

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("granted", ql::datum_t(1.0));
    result_builder.overwrite(
        "permissions_changes",
        make_replacement_pair(old_permissions, new_permissions));
    *result_out = std::move(result_builder).to_datum();
    return true;
}

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_GRANT_HPP
