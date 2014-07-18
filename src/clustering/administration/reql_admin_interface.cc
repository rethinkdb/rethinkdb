#include "clustering/administration/reql_admin_interface.hpp"

#include "clustering/administration/suggester.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "rpc/semilattice/view/field.hpp"

cluster_reql_admin_interface_t::cluster_reql_admin_interface_t(
        uuid_u mmi,
        boost::shared_ptr<
            semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > s,
        clone_ptr_t<watchable_t<
            change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > d
        ) :
    my_machine_id(mmi),
    semilattices(s),
    directory(d),
    cross_thread_namespace_watchables(get_num_threads()),
    cross_thread_database_watchables(get_num_threads())
{
    for (int thr = 0; thr < get_num_threads(); ++thr) {
        cross_thread_namespace_watchables[thr].init(
            new cross_thread_watchable_variable_t<cow_ptr_t<namespaces_semilattice_metadata_t> >(
                clone_ptr_t<semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
                    (new semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> >(
                        metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, semilattices))), threadnum_t(thr)));

        cross_thread_database_watchables[thr].init(
            new cross_thread_watchable_variable_t<databases_semilattice_metadata_t>(
                clone_ptr_t<semilattice_watchable_t<databases_semilattice_metadata_t> >
                    (new semilattice_watchable_t<databases_semilattice_metadata_t>(
                        metadata_field(&cluster_semilattice_metadata_t::databases, semilattices))), threadnum_t(thr)));
    }
}

static bool check_metadata_status(metadata_search_status_t status,
                                  const char *entity_type,
                                  const std::string &entity_name,
                                  bool expect_present,
                                  std::string *error_out) {
    switch (status) {
        case METADATA_SUCCESS: {
            if (expect_present) {
                return true;
            } else {
                *error_out = strprintf("%s `%s` already exists.",
                    entity_type, entity_name.c_str());
                return false;
            }
        }
        case METADATA_ERR_MULTIPLE: {
            if (expect_present) {
                *error_out = strprintf("%s `%s` is ambiguous; there are multiple "
                    "entities with that name.", entity_type, entity_name.c_str());
            } else {
                *error_out = strprintf("%s `%s` already exists.",
                    entity_type, entity_name.c_str());
            }
            return false;
        }
        case METADATA_CONFLICT: {
            *error_out = "There is a metadata conflict; please resolve it.";
            return false;
        }
        case METADATA_ERR_NONE: {
            if (expect_present) {
                *error_out = strprintf("%s `%s` does not exist.",
                    entity_type, entity_name.c_str());
                return false;
            } else {
                return true;
            }
        default: unreachable();
        }
    }
}

bool cluster_reql_admin_interface_t::db_create(const name_string_t &name,
        signal_t *interruptor, std::string *error_out) {
    cluster_semilattice_metadata_t metadata;
    {
        on_thread_t thread_switcher(semilattices->home_thread());
        metadata = semilattices->get();
        metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
            &metadata.databases.databases);

        metadata_search_status_t status;
        db_searcher.find_uniq(name, &status);
        if (!check_metadata_status(status, "Database", name.str(), false, error_out)) {
            return false;
        }

        database_semilattice_metadata_t db;
        db.name = vclock_t<name_string_t>(name, my_machine_id);
        metadata.databases.databases.insert(
            std::make_pair(generate_uuid(), make_deletable(db)));

        semilattices->join(metadata);
        metadata = semilattices->get();
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    return true;
}

bool cluster_reql_admin_interface_t::db_drop(const name_string_t &name,
        signal_t *interruptor, std::string *error_out) {
    cluster_semilattice_metadata_t metadata;
    {
        on_thread_t thread_switcher(semilattices->home_thread());
        metadata = semilattices->get();
        metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
            &metadata.databases.databases);

        metadata_search_status_t status;
        auto it = db_searcher.find_uniq(name, &status);
        if (!check_metadata_status(status, "Database", name.str(), true, error_out)) {
            return false;
        }

        /* Delete the database */
        deletable_t<database_semilattice_metadata_t> *db_metadata = &it->second;
        guarantee(!db_metadata->is_deleted());
        db_metadata->mark_deleted();

        /* Delete all of the tables in the database */
        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
            &metadata.rdb_namespaces);
        metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
            &ns_change.get()->namespaces);
        namespace_predicate_t pred(&it->first);
        for (auto it2 = ns_searcher.find_next(ns_searcher.begin(), pred);
                  it2 != ns_searcher.end();
                  it2 = ns_searcher.find_next(++it2, pred)) {
            guarantee(!it2->second.is_deleted());
            it2->second.mark_deleted();
        }

        semilattices->join(metadata);
        metadata = semilattices->get();
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    return true;
}

bool cluster_reql_admin_interface_t::db_list(
        UNUSED signal_t *interruptor, std::set<name_string_t> *names_out,
        UNUSED std::string *error_out) {
    databases_semilattice_metadata_t db_metadata;
    get_databases_metadata(&db_metadata);
    const_metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
        &db_metadata.databases);
    for (auto it = db_searcher.find_next(db_searcher.begin());
              it != db_searcher.end();
              it = db_searcher.find_next(++it)) {
        guarantee(!it->second.is_deleted());
        if (it->second.get_ref().name.in_conflict()) {
            continue;
        }
        names_out->insert(it->second.get_ref().name.get());
    }

    return true;
}

bool cluster_reql_admin_interface_t::db_find(const name_string_t &name,
        UNUSED signal_t *interruptor, counted_t<const ql::db_t> *db_out,
        std::string *error_out) {
    /* Find the specified database */
    databases_semilattice_metadata_t db_metadata;
    get_databases_metadata(&db_metadata);
    const_metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
        &db_metadata.databases);
    metadata_search_status_t status;
    auto it = db_searcher.find_uniq(name, &status);
    if (!check_metadata_status(status, "Database", name.str(), true, error_out)) {
        return false;
    }
    *db_out = make_counted<const ql::db_t>(it->first, name.str());
    return true;
}

bool cluster_reql_admin_interface_t::table_create(const name_string_t &name,
        counted_t<const ql::db_t> db, const boost::optional<name_string_t> &primary_dc,
        bool hard_durability, const std::string &primary_key, signal_t *interruptor,
        uuid_u *namespace_id_out, std::string *error_out) {
    cluster_semilattice_metadata_t metadata;
    {
        on_thread_t thread_switcher(semilattices->home_thread());
        metadata = semilattices->get();

        /* Find the specified datacenter */
        uuid_u dc_id;
        if (primary_dc) {
            metadata_searcher_t<datacenter_semilattice_metadata_t> dc_searcher(
                &metadata.datacenters.datacenters);
            metadata_search_status_t status;
            auto it = dc_searcher.find_uniq(*primary_dc, &status);
            if (!check_metadata_status(status, "Datacenter", primary_dc->str(), true,
                    error_out)) return false;
            dc_id = it->first;
        } else {
            dc_id = nil_uuid();
        }

        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
            &metadata.rdb_namespaces);
        metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
            &ns_change.get()->namespaces);

        /* Make sure there isn't an existing table with the same name */
        {
            metadata_search_status_t status;
            namespace_predicate_t pred(&name, &db->id);
            ns_searcher.find_uniq(pred, &status);
            if (!check_metadata_status(status, "Table", db->name + "." + name.str(),
                false, error_out)) return false;
        }

        /* Construct a description of the new namespace */
        namespace_semilattice_metadata_t table = new_namespace(
                my_machine_id, db->id, dc_id, name, primary_key);
        std::map<datacenter_id_t, ack_expectation_t> *ack_map =
                &table.ack_expectations.get_mutable();
        for (auto pair : *ack_map) {
            pair.second = ack_expectation_t(pair.second.expectation(), hard_durability);
        }
        table.ack_expectations.upgrade_version(my_machine_id);

        *namespace_id_out = generate_uuid();
        ns_change.get()->namespaces.insert(
            std::make_pair(*namespace_id_out, make_deletable(table)));

        try {
            fill_in_blueprints(&metadata,
                               directory->get().get_inner(),
                               my_machine_id,
                               boost::optional<namespace_id_t>());
        } catch (const missing_machine_exc_t &exc) {
            *error_out = strprintf(
                "Could not configure table: %s Table was not created.", exc.what());
            return false;
        }

        semilattices->join(metadata);
        metadata = semilattices->get();
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    return true;
}

bool cluster_reql_admin_interface_t::table_drop(const name_string_t &name,
        counted_t<const ql::db_t> db, signal_t *interruptor, std::string *error_out) {
    cluster_semilattice_metadata_t metadata;
    {
        on_thread_t thread_switcher(semilattices->home_thread());
        metadata = semilattices->get();

        /* Find the specified table */
        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
                &metadata.rdb_namespaces);
        metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
                &ns_change.get()->namespaces);
        metadata_search_status_t status;
        namespace_predicate_t pred(&name, &db->id);
        metadata_searcher_t<namespace_semilattice_metadata_t>::iterator
            ns_metadata = ns_searcher.find_uniq(pred, &status);
        if (!check_metadata_status(status, "Table", db->name + "." + name.str(), true,
                error_out)) return false;
        guarantee(!ns_metadata->second.is_deleted());

        /* Delete the table. */
        ns_metadata->second.mark_deleted();

        semilattices->join(metadata);
        metadata = semilattices->get();
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    return true;
}

bool cluster_reql_admin_interface_t::table_list(counted_t<const ql::db_t> db,
        UNUSED signal_t *interruptor, std::set<name_string_t> *names_out,
        UNUSED std::string *error_out) {

    cow_ptr_t<namespaces_semilattice_metadata_t> ns_metadata = get_namespaces_metadata();
    const_metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
        &ns_metadata->namespaces);
    namespace_predicate_t pred(&db->id);
    for (auto it = ns_searcher.find_next(ns_searcher.begin(), pred);
              it != ns_searcher.end();
              it = ns_searcher.find_next(++it, pred)) {
        guarantee(!it->second.is_deleted());
        if (it->second.get_ref().name.in_conflict()) {
            /* Maybe we should raise an error instead of silently ignoring the table with
            the conflicted name */
            continue;
        }
        names_out->insert(it->second.get_ref().name.get());
    }

    return true;
}

bool cluster_reql_admin_interface_t::table_find(const name_string_t &name,
        counted_t<const ql::db_t> db, UNUSED signal_t *interruptor,
        uuid_u *namespace_id_out, std::string *primary_key_out, std::string *error_out) {

    /* Find the specified table */
    cow_ptr_t<namespaces_semilattice_metadata_t> namespaces_metadata
        = get_namespaces_metadata();
    const_metadata_searcher_t<namespace_semilattice_metadata_t>
        ns_searcher(&namespaces_metadata.get()->namespaces);
    namespace_predicate_t pred(&name, &db->id);
    metadata_search_status_t status;
    auto ns_metadata_it = ns_searcher.find_uniq(pred, &status);
    if (!check_metadata_status(status, "Table", db->name + "." + name.str(), true,
            error_out)) return false;

    guarantee(!ns_metadata_it->second.is_deleted());
    *namespace_id_out = ns_metadata_it->first;
    guarantee(!ns_metadata_it->second.get_ref().primary_key.in_conflict());
    *primary_key_out = ns_metadata_it->second.get_ref().primary_key.get();
    return true;
}

/* Checks that divisor is indeed a divisor of multiple. */
template <class T>
bool is_joined(const T &multiple, const T &divisor) {
    T cpy = multiple;

    semilattice_join(&cpy, divisor);
    return cpy == multiple;
}

void cluster_reql_admin_interface_t::wait_for_metadata_to_propagate(
        const cluster_semilattice_metadata_t &metadata, signal_t *interruptor) {
    clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
        ns_watchable = get_namespaces_watchable();
    ns_watchable->run_until_satisfied(
            [&] (const cow_ptr_t<namespaces_semilattice_metadata_t> &md) -> bool
                { return is_joined(md, metadata.rdb_namespaces); },
            interruptor);

    clone_ptr_t< watchable_t<databases_semilattice_metadata_t> >
        db_watchable = get_databases_watchable();
    db_watchable->run_until_satisfied(
            [&] (const databases_semilattice_metadata_t &md) -> bool
                { return is_joined(md, metadata.databases); },
            interruptor);
}

template <class T>
void copy_value(const T *in, T *out) {
    *out = *in;
}

clone_ptr_t< watchable_t< cow_ptr_t<namespaces_semilattice_metadata_t> > >
cluster_reql_admin_interface_t::get_namespaces_watchable() {
    int threadnum = get_thread_id().threadnum;
    r_sanity_check(cross_thread_namespace_watchables[threadnum].has());
    return cross_thread_namespace_watchables[threadnum]->get_watchable();
}

cow_ptr_t<namespaces_semilattice_metadata_t>
cluster_reql_admin_interface_t::get_namespaces_metadata() {
    int threadnum = get_thread_id().threadnum;
    r_sanity_check(cross_thread_namespace_watchables[threadnum].has());
    cow_ptr_t<namespaces_semilattice_metadata_t> ret;
    cross_thread_namespace_watchables[threadnum]->apply_read(
            std::bind(&copy_value< cow_ptr_t<namespaces_semilattice_metadata_t> >,
                      ph::_1, &ret));
    return ret;
}

clone_ptr_t< watchable_t<databases_semilattice_metadata_t> >
cluster_reql_admin_interface_t::get_databases_watchable() {
    int threadnum = get_thread_id().threadnum;
    r_sanity_check(cross_thread_database_watchables[threadnum].has());
    return cross_thread_database_watchables[threadnum]->get_watchable();
}

void cluster_reql_admin_interface_t::get_databases_metadata(
        databases_semilattice_metadata_t *out) {
    int threadnum = get_thread_id().threadnum;
    r_sanity_check(cross_thread_database_watchables[threadnum].has());
    cross_thread_database_watchables[threadnum]->apply_read(
            std::bind(&copy_value<databases_semilattice_metadata_t>,
                      ph::_1, out));
}

