#include "clustering/administration/suggester.hpp"
#include "rdb_protocol/meta_utils.hpp"
#include "rdb_protocol/op.hpp"
#include "rpc/directory/read_manager.hpp"

namespace ql {

name_string_t get_name(val_t *val) {
    std::string raw_name = val->as_datum()->as_str();
    name_string_t name;
    bool b = name.assign_value(raw_name);
    rcheck(b, strprintf("name %s invalid (%s)", raw_name.c_str(), valid_char_msg));
    return name;
}

class meta_op_t : public op_term_t {
public:
    meta_op_t(env_t *env, const Term2 *term, argspec_t argspec)
        : op_term_t(env, term, argspec),
          original_thread(get_thread_id()),
          metadata_home_thread(env->semilattice_metadata->home_thread()) {
        on_thread_t rethreader(metadata_home_thread);
        metadata = env->semilattice_metadata->get();

        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t
            ns_change(&metadata.rdb_namespaces);
        ns_searcher.init(
            new metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >(
                &ns_change.get()->namespaces));
        db_searcher.init(
            new metadata_searcher_t<database_semilattice_metadata_t>(
                &metadata.databases.databases));
        dc_searcher.init(
            new metadata_searcher_t<datacenter_semilattice_metadata_t>(
                &metadata.datacenters.datacenters));
    }
protected:
    int original_thread, metadata_home_thread;

    cluster_semilattice_metadata_t metadata;

    scoped_ptr_t<metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> > >
        ns_searcher;
    scoped_ptr_t<metadata_searcher_t<database_semilattice_metadata_t> > db_searcher;
    scoped_ptr_t<metadata_searcher_t<datacenter_semilattice_metadata_t> > dc_searcher;
};

class meta_write_op_t : public meta_op_t {
public:
    meta_write_op_t(env_t *env, const Term2 *term, argspec_t argspec)
        : meta_op_t(env, term, argspec) {
        on_thread_t rethreader(metadata_home_thread);
        rcheck(env->directory_read_manager,
               "Cannot nest meta operations inside queries.");
        guarantee(env->directory_read_manager->home_thread() == metadata_home_thread);
        directory_metadata = env->directory_read_manager->get_root_view();
    }
private:
    virtual std::string write_eval_impl() = 0;
    virtual val_t *eval_impl() {
        std::string op; {
            on_thread_t rethreader(metadata_home_thread);
            op = write_eval_impl();
        }
        datum_t *res = env->add_ptr(new datum_t(datum_t::R_OBJECT));
        const datum_t *num_1 = env->add_ptr(new datum_t(1));
        UNUSED bool b = res->add(op, num_1);
        return new_val(res);

    }
protected:
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >
        directory_metadata;
};

class db_term_t : public meta_op_t {
public:
    db_term_t(env_t *env, const Term2 *term) : meta_op_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        name_string_t db_name = get_name(arg(0));
        return new_val(meta_get_uuid(db_searcher.get(), db_name,
                                     "FIND_DB " + db_name.str()));
    }
    RDB_NAME("db")
};

class db_create_term_t : public meta_write_op_t {
public:
    db_create_term_t(env_t *env, const Term2 *term) :
        meta_write_op_t(env, term, argspec_t(1)) { }
private:
    virtual std::string write_eval_impl() {
        name_string_t db_name = get_name(arg(0));
        metadata_search_status_t status;

        // Ensure database doesn't already exist.
        db_searcher->find_uniq(db_name, &status);
        meta_check(status, METADATA_ERR_NONE, "DB_CREATE " + db_name.str());

        // Create database, insert into metadata, then join into real metadata.
        database_semilattice_metadata_t db;
        db.name = vclock_t<name_string_t>(db_name, env->this_machine);
        metadata.databases.databases.insert(std::make_pair(generate_uuid(), db));
        try {
            fill_in_blueprints(&metadata, directory_metadata->get(),
                               env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            throw exc_t(e.what());
        }
        env->semilattice_metadata->join(metadata);

        return "created";
    }
    RDB_NAME("db_create")
};

class db_drop_term_t : public meta_write_op_t {
public:
    db_drop_term_t(env_t *env, const Term2 *term) :
        meta_write_op_t(env, term, argspec_t(1)) { }
private:
    virtual std::string write_eval_impl() {
        name_string_t db_name = get_name(arg(0));
        metadata_search_status_t status;

        // Get database metadata.
        metadata_searcher_t<database_semilattice_metadata_t>::iterator
            db_metadata = db_searcher->find_uniq(db_name, &status);
        meta_check(status, METADATA_SUCCESS, "DB_DROP " + db_name.str());
        guarantee(!db_metadata->second.is_deleted());
        uuid_t db_id = db_metadata->first;

        // Delete all tables in database.
        namespace_predicate_t pred(&db_id);
        for (metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
                 ::iterator it = ns_searcher->find_next(ns_searcher->begin(), pred);
             it != ns_searcher->end(); it = ns_searcher->find_next(++it, pred)) {
            guarantee(!it->second.is_deleted());
            it->second.mark_deleted();
        }

        // Delete database
        db_metadata->second.mark_deleted();

        // Join
        try {
            fill_in_blueprints(&metadata, directory_metadata->get(),
                               env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            throw exc_t(e.what());
        }
        env->semilattice_metadata->join(metadata);

        return "dropped";
    }
    RDB_NAME("db_drop")
};


class db_list_term_t : public meta_op_t {
public:
    db_list_term_t(env_t *env, const Term2 *term) :
        meta_op_t(env, term, argspec_t(0)) { }
private:
    virtual val_t *eval_impl() {
        datum_t *arr = env->add_ptr(new datum_t(datum_t::R_ARRAY));
        for (metadata_searcher_t<database_semilattice_metadata_t>::iterator
                 it = db_searcher->find_next(db_searcher->begin());
             it != db_searcher->end(); it = db_searcher->find_next(++it)) {
            guarantee(!it->second.is_deleted());
            if (it->second.get().name.in_conflict()) continue;
            std::string s = it->second.get().name.get().c_str();
            arr->add(env->add_ptr(new datum_t(s)));
        }
        return new_val(arr);
    }
    RDB_NAME("db_list")
};


static const char *const table_optargs[] = {"use_outdated"};
class table_term_t : public op_term_t {
public:
    table_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(2), LEGAL_OPTARGS(table_optargs)) { }
private:
    virtual val_t *eval_impl() {
        val_t *t = optarg("use_outdated", 0);
        bool use_outdated = t ? t->as_datum()->as_bool() : false;
        uuid_t db = arg(0)->as_db();
        std::string name = arg(1)->as_datum()->as_str();
        return new_val(new table_t(env, db, name, use_outdated));
    }
    RDB_NAME("table")
};

class get_term_t : public op_term_t {
public:
    get_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        table_t *table = arg(0)->as_table();
        const datum_t *pkey = arg(1)->as_datum();
        const datum_t *row = table->get_row(pkey);
        return new_val(row, table);
    }
    RDB_NAME("get")
};
} // ql
