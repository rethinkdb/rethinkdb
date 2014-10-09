#ifndef RDB_PROTOCOL_TERMS_ARR_HPP_
#define RDB_PROTOCOL_TERMS_ARR_HPP_

template <class> class scoped_ptr_t;
namespace ql {
class val_t;
class term_t;
class scope_env_t;

scoped_ptr_t<val_t> nth_term_impl(const term_t *term, scope_env_t *env,
                                  scoped_ptr_t<val_t> aggregate,
                                  const scoped_ptr_t<val_t> &index);
}  // namespace ql

#endif  // RDB_PROTOCOL_TERMS_ARR_HPP_
