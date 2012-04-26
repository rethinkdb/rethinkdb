#ifndef LENS_HPP_
#define LENS_HPP_

#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "containers/clone_ptr.hpp"
#include <map>

template<class inner_t, class outer_t>
class read_lens_t {
public:
    virtual ~read_lens_t() { }
    virtual inner_t get(const outer_t &) const = 0;
    virtual read_lens_t *clone() const = 0;
};

template<class inner_t, class outer_t>
class readwrite_lens_t : public read_lens_t<inner_t, outer_t> {
public:
    virtual ~readwrite_lens_t() { }
    virtual void set(outer_t *, const inner_t &) const = 0;
    virtual readwrite_lens_t *clone() const = 0;
};

#if 0

template<class inner_t, class outer_t>
clone_ptr_t<readwrite_lens_t<inner_t, outer_t> > field_lens(inner_t outer_t::*field);

template<class inner_t, class outer_t>
clone_ptr_t<readwrite_lens_t<inner_t, outer_t> > ref_fun_lens(const boost::function<inner_t &(outer_t &)> &fun);

/* Crashes if the key is not in the map */
template<class key_t, class value_t>
clone_ptr_t<readwrite_lens_t<value_t, std::map<key_t, value_t> > > assumed_member_lens(const key_t &key);

/* Value is `boost::optional<value_t>()` if key is not in the map */
template<class key_t, class value_t>
clone_ptr_t<readwrite_lens_t<boost::optional<value_t>, std::map<key_t, value_t> > > optional_member_lens(const key_t &key);

/* If input is empty, output is also empty; otherwise, output is sublens applied
to input. */
template<class inner_t, class outer_t>
clone_ptr_t<read_lens_t<boost::optional<inner_t>, boost::optional<outer_t> > > optional_monad_lens(const clone_ptr_t<read_lens_t<inner_t, outer_t> > &sublens);

/* If either of the optionals is nothing return nothing, otherwise return the thing. */
template<class value_t>
clone_ptr_t<read_lens_t<boost::optional<value_t>, boost::optional<boost::optional<value_t> > > > optional_collapser_lens();

/* Return the results of the outer lens applied to the results of the inner lens. */
template <class inner_t, class middle_t, class outer_t>
clone_ptr_t<read_lens_t<inner_t, outer_t> > compose_lens(const clone_ptr_t<read_lens_t<inner_t, middle_t> > &inner, const clone_ptr_t<read_lens_t<middle_t, outer_t> > &outer);

/* Return the value if it's in the map.. otherwise return a default value */
template<class key_t, class value_t>
clone_ptr_t<read_lens_t<value_t, std::map<key_t, value_t> > > default_member_lens(const key_t &key, value_t default_val = value_t());

#endif

#include "lens.tcc"

#endif /* LENS_HPP_ */
