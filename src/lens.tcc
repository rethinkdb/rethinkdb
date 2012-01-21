template<class inner_t, class outer_t>
class field_readwrite_lens_t : public readwrite_lens_t<inner_t, outer_t> {
public:
    field_readwrite_lens_t(inner_t outer_t::*f) : field(f) { }
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
    ref_fun_readwrite_lens_t(const boost::function<inner_t &(outer_t &)> &f) : fun(f) { }
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
    assumed_member_readwrite_lens_t(const key_t &k) : key(k) { }
    value_t get(const std::map<key_t, value_t> &o) const {
        rassert(o.count(key) == 1);
        return o[key];
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
    optional_member_readwrite_lens_t(const key_t &k) : key(k) { }
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
    optional_monad_read_lens_t(const clone_ptr_t<read_lens_t<inner_t, outer_t> > &sl) : sublens(sl) { }
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
