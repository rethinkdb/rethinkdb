template<class inner_t, class outer_t>
class field_readwrite_lens_t : public readwrite_lens_t<inner_t, outer_t> {
public:
    field_readwrite_lens_t(inner_t outer_t::*f) : field(f) { }
    inner_t get(const outer_t &o) {
        return o.*field;
    }
    void set(outer_t *o, const inner_t &i) {
        o->*field = i;
    }
    field_readwrite_lens_t *clone() {
        return new field_readwrite_lens_t(field);
    }
private:
    inner_t outer_t::*field;
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
    inner_t get(const outer_t &o) {
        return fun(const_cast<outer_t &>(o));
    }
    void set(outer_t *o, const inner_t &i) {
        fun(*o) = i;
    }
    ref_fun_readwrite_lens_t *clone() {
        return new ref_fun_readwrite_lens_t(fun);
    }
private:
    boost::function<inner_t &(outer_t &)> fun;
};

template<class inner_t, class outer_t>
clone_ptr_t<readwrite_lens_t<inner_t, outer_t> > ref_fun_lens(const boost::function<inner_t &(outer_t &)> &fun) {
    return clone_ptr_t<readwrite_lens_t<inner_t, outer_t> >(
        new ref_fun_readwrite_lens_t(fun)
        );
}

template<class key_t, class value_t>
class assumed_member_readwrite_lens_t : public readwrite_lens_t<value_t, std::map<key_t, value_t> > {
public:
    assumed_member_readwrite_lens_t(const key_t &k) : key(k) { }
    value_t get(const std::map<key_t, value_t> &o) {
        rassert(o.count(key) == 1);
        return o[key];
    }
    void set(std::map<key_t, value_t> *o, const value_t &i) {
        rassert(o->count(key) == 1);
        (*o)[key] = i;
    }
    assumed_member_readwrite_lens_t *clone() {
        return new assumed_member_readwrite_lens_t(key);
    }
private:
    key_t key;
};

template<class key_t, class value_t>
clone_ptr_t<readwrite_lens_t<value_t, std::map<key_t, value_t> > > assumed_member_lens(const key_t &key) {
    return clone_ptr_t<readwrite_lens_t<value_t, std::map<key_t, value_t> > >(
        new assumed_member_readwrite_lens_t(key)
        );
}

template<class key_t, class value_t>
class optional_member_readwrite_lens_t : public readwrite_lens_t<boost::optional<value_t>, std::map<key_t, value_t> > {
public:
    optional_member_readwrite_lens_t(const key_t &k) : key(k) { }
    boost::optional<value_t> get(const std::map<key_t, value_t> &o) {
        std::map<key_t, value_t>::const_iterator it = o.find(key);
        if (it == o.end()) {
            return boost::optional<value_t>();
        } else {
            return boost::optional<value_t>((*it).second);
        }
    }
    void set(std::map<key_t, value_t> *o, const boost::optional<value_t> &i) {
        if (i) {
            (*o)[key] = i.get();
        } else {
            o->erase(key);
        }
    }
    optional_member_readwrite_lens_t *clone() {
        return new optional_member_readwrite_lens_t(key);
    }
private:
    key_t key;
};

template<class key_t, class value_t>
clone_ptr_t<readwrite_lens_t<boost::optional<value_t>, std::map<key_t, value_t> > > optional_member_lens(const key_t &key) {
    return clone_ptr_t<readwrite_lens_t<boost::optional<value_t>, std::map<key_t, value_t> > >(
        new optional_member_readwrite_lens_t(key)
        );
}

