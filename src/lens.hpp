#ifndef __LENS_HPP__
#define __LENS_HPP__

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/clone_ptr.hpp"

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

#include "lens.tcc"

#endif /* __LENS_HPP__ */
