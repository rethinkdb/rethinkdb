#include "concurrency/watchable.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/cond_var.hpp"
#include "concurrency/wait_any.hpp"

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

inline void pulse_cond_if_not_pulsed(cond_t *c) {
    if (!c->is_pulsed()) {
        c->pulse();
    }
}

template<class value_type>
template<class callable_type>
void watchable_t<value_type>::run_until_satisfied(const callable_type &fun, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    clone_ptr_t<watchable_t<value_type> > clone_this(this->clone());
    while (true) {
        cond_t changed;
        typename watchable_t<value_type>::subscription_t subs(boost::bind(&pulse_cond_if_not_pulsed, &changed));
        {
            typename watchable_t<value_type>::freeze_t freeze(clone_this);
            if (fun(clone_this->get())) {
                return;
            }
            subs.reset(clone_this, &freeze);
        }
        wait_interruptible(&changed, interruptor);
    }
}
