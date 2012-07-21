#include "rpc/semilattice/view/member.hpp"

#include <map>
#include <utility>

#include "errors.hpp"
#include <boost/make_shared.hpp>

/* `semilattice_member_read_view_t` and `semilattice_member_readwrite_view_t`
correspond to some values of a `std::map` contained in another metadata view. */

template<class key_t, class value_t>
class semilattice_member_read_view_t : public semilattice_read_view_t<value_t> {

public:
    semilattice_member_read_view_t(key_t k, boost::shared_ptr<semilattice_read_view_t<std::map<key_t, value_t> > > sv) :
        key(k), superview(sv)
    {
        rassert(superview->get().count(key) != 0);
    }

    value_t get() {
        return superview->get()[key];
    }

    void sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        superview->sync_from(peer, interruptor);
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return superview->get_publisher();
    }

private:
    key_t key;
    boost::shared_ptr<semilattice_read_view_t<std::map<key_t, value_t> > > superview;
    DISABLE_COPYING(semilattice_member_read_view_t);
};

template<class key_t, class value_t>
class semilattice_member_readwrite_view_t : public semilattice_readwrite_view_t<value_t> {

public:
    semilattice_member_readwrite_view_t(key_t k, boost::shared_ptr<semilattice_readwrite_view_t<std::map<key_t, value_t> > > sv) :
        key(k), superview(sv)
    {
        rassert(superview->get().count(key) != 0);
    }

    value_t get() {
        return superview->get()[key];
    }

    void join(const value_t &v) {
        std::map<key_t, value_t> map = superview->get();
        semilattice_join(&map[key], v);
        superview->join(map);
    }

    void sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        superview->sync_from(peer, interruptor);
    }

    void sync_to(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        superview->sync_to(peer, interruptor);
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return superview->get_publisher();
    }

private:
    key_t key;
    boost::shared_ptr<semilattice_readwrite_view_t<std::map<key_t, value_t> > > superview;
    DISABLE_COPYING(semilattice_member_readwrite_view_t);
};

template<class key_t, class value_t>
boost::shared_ptr<semilattice_read_view_t<value_t> > metadata_member(
    key_t key,
    boost::shared_ptr<semilattice_read_view_t<std::map<key_t, value_t> > > outer)
{
    return boost::make_shared<semilattice_member_read_view_t<key_t, value_t> >(
        key, outer);
}

template<class key_t, class value_t>
boost::shared_ptr<semilattice_readwrite_view_t<value_t> > metadata_member(
    key_t key,
    boost::shared_ptr<semilattice_readwrite_view_t<std::map<key_t, value_t> > > outer)
{
    return boost::make_shared<semilattice_member_readwrite_view_t<key_t, value_t> >(
        key, outer);
}

template<class key_t, class value_t>
boost::shared_ptr<semilattice_readwrite_view_t<value_t> > metadata_new_member(
    key_t key,
    boost::shared_ptr<semilattice_readwrite_view_t<std::map<key_t, value_t> > > outer)
{
    rassert(outer->get().count(key) == 0);
    std::map<key_t, value_t> new_value;
    new_value.insert(std::pair<key_t, value_t>(key, value_t()));
    outer->join(new_value);
    return metadata_member(key, outer);
}
