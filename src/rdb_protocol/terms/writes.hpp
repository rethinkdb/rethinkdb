#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

const datum_t *stats_merge(env_t *env, UNUSED const std::string &key,
                           const datum_t *l, const datum_t *r) {
    if (l->get_type() == datum_t::R_NUM && r->get_type() == datum_t::R_NUM) {
        return env->add_ptr(new datum_t(l->as_num() + r->as_num()));
    }
    r_sanity_check(l->get_type() == datum_t::R_STR && r->get_type() == datum_t::R_STR);
    return l;
}

static const char *const insert_optargs[] = {"upsert"};
class insert_term_t : public op_term_t {
public:
    insert_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(2), LEGAL_OPTARGS(insert_optargs)) { }
private:
    virtual val_t *eval_impl() {
        table_t *t = arg(0)->as_table();
        val_t *upsert_val = optarg("upsert", 0);
        bool upsert = upsert_val ? upsert_val->as_datum()->as_bool() : false;
        if (arg(1)->get_type().is_convertible(val_t::type_t::DATUM)) {
            const datum_t *d = arg(1)->as_datum();
            if (d->get_type() == datum_t::R_OBJECT) {
                return new_val(t->replace(d, d, upsert));
            }
        }
        datum_stream_t *ds = arg(1)->as_seq();

        const datum_t *stats = env->add_ptr(new datum_t(datum_t::R_OBJECT));
        while (const datum_t *d = ds->next()) {
            stats = stats->merge(env, t->replace(d, d, upsert), stats_merge);
        }
        return new_val(stats);
    }
    RDB_NAME("insert")
};



} // namespace ql
