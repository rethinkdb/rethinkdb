#include "rdb_protocol/op.hpp"
namespace ql {
class db_term_t : public op_term_t {
public:
    db_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        std::string name = arg(0)->as_datum()->as_str();
        return new_val(get_db_uuid(env, name));
    }
    RDB_NAME("DB")
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
