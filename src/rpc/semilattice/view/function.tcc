#ifndef RPC_SEMILATTICE_VIEW_FUNCTION_TCC_
#define RPC_SEMILATTICE_VIEW_FUNCTION_TCC_

#include "rpc/semilattice/view/function.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/function.hpp>

template<class outer_t, class inner_t>
class metadata_function_read_view_t : public semilattice_read_view_t<inner_t> {

public:
    metadata_function_read_view_t(boost::function<inner_t &(outer_t &)> _extractor, boost::shared_ptr<semilattice_read_view_t<outer_t> > o) :
        extractor(_extractor), outer(o) { }

    inner_t get() {
        outer_t outer_val = outer->get();
        return extractor(outer->get());
    }

    void sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        outer->sync_from(peer, interruptor);
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return outer->get_publisher();
    }

private:
    boost::function<inner_t &(outer_t &)> extractor;
    boost::shared_ptr<semilattice_read_view_t<outer_t> > outer;
};

template<class outer_t, class inner_t>
class metadata_function_readwrite_view_t : public semilattice_readwrite_view_t<inner_t> {

public:
    metadata_function_readwrite_view_t(boost::function<inner_t &(outer_t &)> _extractor, boost::shared_ptr<semilattice_readwrite_view_t<outer_t> > o) :
        extractor(_extractor), outer(o) { }

    inner_t get() {
        outer_t outer_val = outer->get();
        return extractor(outer_val);
    }

    void join(const inner_t &new_inner) {
#ifndef NDEBUG
        {
            outer_t value = outer->get(), copy = value;
            semilattice_join(&(extractor(value)), new_inner);
            inner_t expected_value = extractor(value);
            semilattice_join(&copy, value);
            rassert_reviewed(expected_value == extractor(copy), "Function view used on a non direct semilattice.");
        }
#endif  // NDEBUG
        outer_t value = outer->get();
        semilattice_join(&(extractor(value)), new_inner);
        outer->join(value);
    }

    void sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        outer->sync_from(peer, interruptor);
    }

    void sync_to(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
        outer->sync_to(peer, interruptor);
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return outer->get_publisher();
    }

private:
    boost::function<inner_t &(outer_t &)> extractor;
    boost::shared_ptr<semilattice_readwrite_view_t<outer_t> > outer;
};

//some convenient wrappers
template<class outer_t, class inner_t>
boost::shared_ptr<semilattice_read_view_t<inner_t> > metadata_function(
        boost::function<inner_t &(outer_t &)> extractor,
        boost::shared_ptr<semilattice_read_view_t<outer_t> > outer)
{
    return boost::make_shared<metadata_function_read_view_t<outer_t, inner_t> >(
        extractor, outer);
}

template<class outer_t, class inner_t>
boost::shared_ptr<semilattice_readwrite_view_t<inner_t> > metadata_function(
        boost::function<inner_t &(outer_t &)> extractor,
        boost::shared_ptr<semilattice_readwrite_view_t<outer_t> > outer)
{
    return boost::make_shared<metadata_function_readwrite_view_t<outer_t, inner_t> >(
        extractor, outer);
}

#endif  // RPC_SEMILATTICE_VIEW_FUNCTION_TCC_
