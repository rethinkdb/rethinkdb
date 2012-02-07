#ifndef __CONCURRENCY_WATCHABLE_HPP__
#define __CONCURRENCY_WATCHABLE_HPP__

template <class value_t>
class watchable_t {
public:
    class freeze_t {
    public:
        explicit freeze_t(watchable_t *watchable) 
            : mutex_acquisition(watchable->get_mutex_assertion())
        { }

        void assert_is_holding(watchable_t *watchable) {
            mutex_acquisition.assert_is_holding(watchable->get_mutex_assertion());
        }

    private:
        mutex_assertion_t::acq_t mutex_acquisition;
    };

    class subscription_t {
    public:
        explicit subscription_t(boost::function<void()> f)
            : subscription(f)
        { }

        subscription_t(boost::function<void()> f, watchable_t *watchable, freeze_t *freeze) 
            : subscription(f, watchable->get_publisher(freeze))
        { }

        void reset() {
            subscription.reset();
        }

        void reset(watchable_t *watchable, freeze_t *freeze) {
            subscription.reset(watchable->get_publisher(freeze));
        }

    private:
        typename publisher_t<boost::function<void()> >::subscription_t subscription;
    };

    virtual value_t get() = 0;
    virtual ~watchable_t() { }
    virtual publisher_t<boost::function<void()> > *get_publisher(freeze_t *) = 0;
    virtual mutex_assertion_t *get_mutex_assertion() = 0;
};

template <class value_t>
class watchable_impl_t : public watchable_t<value_t> {
public:
    explicit watchable_impl_t(const value_t &_value)
        : value(_value)
    { }

    value_t get() {
        return value;
    }

    publisher_t<boost::function<void()> > *get_publisher(typename watchable_t<value_t>::freeze_t *freeze) {
        freeze->assert_is_holding(this);
        return publisher_controller.get_publisher();
    }

    mutex_assertion_t *get_mutex_assertion() {
        return &mutex_assertion;
    }

    void set_value(const value_t &_value) {
        mutex_assertion_t::acq_t acquisition(get_mutex_assertion());
        value = _value;
        publisher_controller.publish(&watchable_impl_t<value_t>::call);
    }

private:
    static void call(const boost::function<void()> &f) {
        f();
    }

    publisher_controller_t<boost::function<void()> > publisher_controller;
    value_t value;
    mutex_assertion_t mutex_assertion;
};

#endif
