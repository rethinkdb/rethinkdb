#include "logger.hpp"

#if 0

template<class inner_t, class outer_t>
class field_readwrite_lens_t : public readwrite_lens_t<inner_t, outer_t> {
public:
    explicit field_readwrite_lens_t(inner_t outer_t::*f) : field(f) { }
    inner_t get(const outer_t &o) const {
        return o.*field;
    }
    void set(outer_t *o, const inner_t &i) const {
        o->*field = i;
    }
    field_readwrite_lens_t *clone() const {
        return new field_readwrite_lens_t(field);
    }
private:
    inner_t outer_t::*const field;
};

template<class inner_t, class outer_t>
clone_ptr_t<readwrite_lens_t<inner_t, outer_t> > field_lens(inner_t outer_t::*field) {
    return clone_ptr_t<readwrite_lens_t<inner_t, outer_t> >(
        new field_readwrite_lens_t<inner_t, outer_t>(field)
        );
}

template<class inner_t, class outer_t>
class ref_fun_readwrite_lens_t : public readwrite_lens_t<inner_t, outer_t> {
public:
    explicit ref_fun_readwrite_lens_t(const boost::function<inner_t &(outer_t &)> &f) : fun(f) { }
    inner_t get(const outer_t &o) const {
        return fun(const_cast<outer_t &>(o));
    }
    void set(outer_t *o, const inner_t &i) const {
        fun(*o) = i;
    }
    ref_fun_readwrite_lens_t *clone() const {
        return new ref_fun_readwrite_lens_t(fun);
    }
private:
    boost::function<inner_t &(outer_t &)> fun;
};

template<class inner_t, class outer_t>
clone_ptr_t<readwrite_lens_t<inner_t, outer_t> > ref_fun_lens(const boost::function<inner_t &(outer_t &)> &fun) {
    return clone_ptr_t<readwrite_lens_t<inner_t, outer_t> >(
        new ref_fun_readwrite_lens_t<inner_t, outer_t>(fun)
        );
}

template<class key_t, class value_t>
class assumed_member_readwrite_lens_t : public readwrite_lens_t<value_t, std::map<key_t, value_t> > {
public:
    explicit assumed_member_readwrite_lens_t(const key_t &k) : key(k) { }
    value_t get(const std::map<key_t, value_t> &o) const {
        rassert(o.count(key) == 1);
        return o.find(key)->second;
    }
    void set(std::map<key_t, value_t> *o, const value_t &i) const {
        rassert(o->count(key) == 1);
        (*o)[key] = i;
    }
    assumed_member_readwrite_lens_t *clone() const {
        return new assumed_member_readwrite_lens_t(key);
    }
private:
    const key_t key;
};

template<class key_t, class value_t>
clone_ptr_t<readwrite_lens_t<value_t, std::map<key_t, value_t> > > assumed_member_lens(const key_t &key) {
    return clone_ptr_t<readwrite_lens_t<value_t, std::map<key_t, value_t> > >(
        new assumed_member_readwrite_lens_t<key_t, value_t>(key)
        );
}

template<class key_t, class value_t>
class optional_member_readwrite_lens_t : public readwrite_lens_t<boost::optional<value_t>, std::map<key_t, value_t> > {
public:
    explicit optional_member_readwrite_lens_t(const key_t &k) : key(k) { }
    boost::optional<value_t> get(const std::map<key_t, value_t> &o) const {
        typename std::map<key_t, value_t>::const_iterator it = o.find(key);
        if (it == o.end()) {
            return boost::optional<value_t>();
        } else {
            return boost::optional<value_t>((*it).second);
        }
    }
    void set(std::map<key_t, value_t> *o, const boost::optional<value_t> &i) const {
        if (i) {
            (*o)[key] = i.get();
        } else {
            o->erase(key);
        }
    }
    optional_member_readwrite_lens_t *clone() const {
        return new optional_member_readwrite_lens_t(key);
    }
private:
    const key_t key;
};

template<class key_t, class value_t>
clone_ptr_t<readwrite_lens_t<boost::optional<value_t>, std::map<key_t, value_t> > > optional_member_lens(const key_t &key) {
    return clone_ptr_t<readwrite_lens_t<boost::optional<value_t>, std::map<key_t, value_t> > >(
        new optional_member_readwrite_lens_t<key_t, value_t>(key)
        );
}

template<class inner_t, class outer_t>
class optional_monad_read_lens_t : public read_lens_t<boost::optional<inner_t>, boost::optional<outer_t> > {
public:
    explicit optional_monad_read_lens_t(const clone_ptr_t<read_lens_t<inner_t, outer_t> > &sl) : sublens(sl) { }
    boost::optional<inner_t> get(const boost::optional<outer_t> &o) const {
        if (o) {
            return boost::optional<inner_t>(sublens->get(o.get()));
        } else {
            return boost::optional<inner_t>();
        }
    }
    optional_monad_read_lens_t *clone() const {
        return new optional_monad_read_lens_t(sublens);
    }
private:
    const clone_ptr_t<read_lens_t<inner_t, outer_t> > sublens;
};

template<class inner_t, class outer_t>
clone_ptr_t<read_lens_t<boost::optional<inner_t>, boost::optional<outer_t> > > optional_monad_lens(const clone_ptr_t<read_lens_t<inner_t, outer_t> > &sublens) {
    return clone_ptr_t<read_lens_t<boost::optional<inner_t>, boost::optional<outer_t> > >(
        new optional_monad_read_lens_t<inner_t, outer_t>(sublens)
        );
}

template <class value_t>
class optional_collapser_read_lens_t : public read_lens_t<boost::optional<value_t>, boost::optional<boost::optional<value_t> > >
{
public:
    boost::optional<value_t> get(const boost::optional<boost::optional<value_t> > &val) const {
        if (val) {
            return *val;
        } else {
            return boost::optional<value_t>();
        }
    }
    optional_collapser_read_lens_t *clone() const {
        return new optional_collapser_read_lens_t;
    }
};

/* If either of the optionals is nothing return nothing, otherwise return the thing. */
template<class value_t>
clone_ptr_t<read_lens_t<boost::optional<value_t>, boost::optional<boost::optional<value_t> > > > optional_collapser_lens() {
    return clone_ptr_t<read_lens_t<boost::optional<value_t>, boost::optional<boost::optional<value_t> > > >(
            new optional_collapser_read_lens_t<value_t>);
}

template <class inner_t, class middle_t, class outer_t>
class compose_read_lens_t : public read_lens_t<inner_t, outer_t> {
public:
    compose_read_lens_t(const clone_ptr_t<read_lens_t<inner_t, middle_t> > &_inner, const clone_ptr_t<read_lens_t<middle_t, outer_t> > &_outer)
        : inner(_inner), outer(_outer)
    { }

    inner_t get(const outer_t &outer_value) const {
        return inner->get(outer->get(outer_value));
    }

    compose_read_lens_t *clone() const {
        return new compose_read_lens_t(inner, outer);
    }

private:
    clone_ptr_t<read_lens_t<inner_t, middle_t> > inner;
    clone_ptr_t<read_lens_t<middle_t, outer_t> > outer;
};

/* Return the results of the outer lens applied to the results of the inner lens. */
template <class inner_t, class middle_t, class outer_t>
clone_ptr_t<read_lens_t<inner_t, outer_t> > compose_lens(const clone_ptr_t<read_lens_t<inner_t, middle_t> > &inner, const clone_ptr_t<read_lens_t<middle_t, outer_t> > &outer) {
    return clone_ptr_t<read_lens_t<inner_t, outer_t> >(new compose_read_lens_t<inner_t, middle_t, outer_t>(inner, outer));
}

/* Return the value if it's in the map.. otherwise return a default value */
template<class key_t, class value_t>
class default_member_read_lens_t : public read_lens_t<value_t, std::map<key_t, value_t> > {
public:
    explicit default_member_read_lens_t(const key_t &k, value_t _default_val = value_t()) : key(k), default_val(_default_val) { }
    value_t get(const std::map<key_t, value_t> &o) const {
        if (o.find(key) == o.end()) {
            return default_val;
        }
        return o.find(key)->second;
    }

    default_member_read_lens_t *clone() const {
        return new default_member_read_lens_t(key, default_val);
    }
private:
    const key_t key;
    value_t default_val;
};

template<class key_t, class value_t>
clone_ptr_t<read_lens_t<value_t, std::map<key_t, value_t> > > default_member_lens(const key_t &key, value_t default_val) {
    return clone_ptr_t<read_lens_t<value_t, std::map<key_t, value_t> > >(
        new default_member_read_lens_t<key_t, value_t>(key, default_val)
        );
}

#endif
