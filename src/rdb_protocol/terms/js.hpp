#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/js.hpp"

namespace ql {

class javascript_term_t : public op_term_t {
public:
    javascript_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    
    /** TODO(bill)
     * This implementation may need to move to the constructor. The class can then remember
     * the resulting value and simply return it in eval_impl. Why? well, a func_term_t is
     * not a val_t, it's a term_t (duh!). JS terms may evaluate to func's or to simple values.
     * This polymorphism can't be resolved until the JS expression is evaluated and we know
     * what the return type actually is. If we evaluate the expression we might as well
     * remember any primitive value returned. Note that actual func expressions will have to
     * be passed back to the JS interpreter (pre compiled though) when the FunCall containing
     * it is evaluated. Thus eval_impl will either simply return a remembered datum value
     * or execute it's remembered JS function expression with the arguments from the FunCall.
     */

    virtual val_t *eval_impl() {
        std::string source = arg(0)->as_datum()->as_str();

        boost::shared_ptr<js::runner_t> js = env->get_js_runner();
        std::string errmsg;
        boost::shared_ptr<scoped_cJSON_t> result = js->eval(source, &errmsg);

        if (!result) {
            throw exc_t(errmsg);
        }

        return new_val(new datum_t(result, env));
    }
    RDB_NAME("javascript");
};

}
