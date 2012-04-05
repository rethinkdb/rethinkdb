template <class outer_type, class callable_type>
class subview_watchable_t : public watchable_t<typename boost::result_of<callable_type(outer_type)>::type> {
public:
    subview_watchable_t(const callable_type &l, watchable_t<outer_type> *p) :
        lens(l), parent(p->clone()) { }

    subview_watchable_t *clone() {
        return new subview_watchable_t(lens, parent.get());
    }

    typename boost::result_of<callable_type(outer_type)>::type get() {
        return lens(parent->get());
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return parent->get_publisher();
    }

    rwi_lock_assertion_t *get_rwi_lock_assertion() {
        return parent->get_rwi_lock_assertion();
    }

private:
    callable_type lens;
    clone_ptr_t<watchable_t<outer_type> > parent;
};

template<class value_type>
template<class callable_type>
clone_ptr_t<watchable_t<typename boost::result_of<callable_type(value_type)>::type> > watchable_t<value_type>::subview(const callable_type &lens) {
    return clone_ptr_t<watchable_t<typename boost::result_of<callable_type(value_type)>::type> >(
        new subview_watchable_t<value_type, callable_type>(
            lens, this
            ));
}
