#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"
namespace ql {

class datum_term_t : public term_t {
public:
    datum_term_t(env_t *env, const Datum *datum)
        : term_t(env), raw_val(new val_t(new datum_t(datum), this)) {
        guarantee(raw_val.has());
    }
    virtual val_t *eval_impl() { return raw_val.get(); }
    virtual const char *name() const { return "datum"; }
private:
    scoped_ptr_t<val_t> raw_val;
};
}
