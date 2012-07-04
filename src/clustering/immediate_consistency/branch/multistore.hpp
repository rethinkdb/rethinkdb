#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_

#include <vector>

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "concurrency/fifo_enforcer.hpp"
#include "containers/scoped.hpp"

namespace boost { template <class> class function; }
class binary_blob_t;
template <class> class metainfo_checker_t;
template <class> class multistore_send_backfill_should_backfill_t;
class mutex_t;
template <class, class> class region_map_t;
template <class> class store_view_t;
template <class> class store_subview_t;
class order_token_t;
class version_range_t;

template <class> struct new_and_metainfo_checker_t;

template <class protocol_t>
class multistore_ptr_t {
public:
    // We don't get ownership of the store_view_t pointers themselves.
    multistore_ptr_t(store_view_t<protocol_t> **store_views, int num_store_views, const typename protocol_t::region_t &region_mask = protocol_t::region_t::universe());

    // Creates a multistore_ptr_t that depends on the lifetime of the
    // inner multistore_ptr_t.
    multistore_ptr_t(multistore_ptr_t<protocol_t> *inner, const typename protocol_t::region_t &region_mask);

    // deletes the store_subview_t objects.
    ~multistore_ptr_t();

    typename protocol_t::region_t get_multistore_joined_region() const;

    int num_stores() const { return store_views.size(); }

    void new_read_tokens(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens_out, int num_stores_assertion);

    void new_write_tokens(boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens_out, int num_stores_assertion);

    void new_particular_write_tokens(int *indices, int num_indices, boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens);

    // TODO: I'm pretty sure every time we call this function we are
    // doing something stupid.
    region_map_t<protocol_t, version_range_t>
    get_all_metainfos(order_token_t order_token,
                      boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                      int num_read_tokens,
		      signal_t *interruptor);

    typename protocol_t::region_t get_region(int i) const;
    store_view_t<protocol_t> *get_store_view(int i) const;

    // TODO: Perhaps all uses of set_all_metainfos are stupid, too.
    // See get_all_metainfos.  This is the opposite of
    // get_all_metainfos but is a bit more scary.
    void set_all_metainfos(const region_map_t<protocol_t, binary_blob_t> &new_metainfo,
                           order_token_t order_token,
                           boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                           int num_write_tokens,
                           signal_t *interruptor);

    bool send_multistore_backfill(const region_map_t<protocol_t, state_timestamp_t> &start_point,
                                  const boost::function<bool(const typename protocol_t::store_t::metainfo_t &)> &should_backfill,
                                  const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
                                  typename protocol_t::backfill_progress_t *progress,
                                  boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                                  int num_stores_assertion,
                                  signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    typename protocol_t::read_response_t read(DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
                                              const typename protocol_t::read_t &read,
                                              order_token_t order_token,
                                              boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                                              int num_stores_assertion,
                                              signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    typename protocol_t::write_response_t write(DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
                                                const typename protocol_t::store_t::metainfo_t& new_metainfo,
                                                const typename protocol_t::write_t &write,
                                                transition_timestamp_t timestamp,
                                                order_token_t order_token,
                                                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                                int num_stores_assertion,
                                                signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void reset_all_data(const typename protocol_t::region_t &subregion,
                        const typename protocol_t::store_t::metainfo_t &new_metainfo,
                        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                        int num_stores_assertion,
                        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);



private:
    // Used by send_multistore_backfill.
    void single_shard_backfill(int i,
                               multistore_send_backfill_should_backfill_t<protocol_t> *helper,
                               const region_map_t<protocol_t, state_timestamp_t> &start_point,
                               const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
                               typename protocol_t::backfill_progress_t *progress,
                               boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                               signal_t *interruptor) THROWS_NOTHING;

    void single_shard_read(int i,
                           DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
                           const typename protocol_t::read_t &read,
                           order_token_t order_token,
                           boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                           std::vector<typename protocol_t::read_response_t> *responses,
                           signal_t *interruptor) THROWS_NOTHING;



    void single_shard_write(int i,
                            const new_and_metainfo_checker_t<protocol_t> &metainfo,
                            const typename protocol_t::write_t &write,
                            transition_timestamp_t timestamp,
                            order_token_t order_token,
                            boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                            std::vector<typename protocol_t::write_response_t> *responses,
                            signal_t *interruptor) THROWS_NOTHING;

    void single_shard_reset_all_data(int i,
                                     const typename protocol_t::region_t &subregion,
                                     const typename protocol_t::store_t::metainfo_t &new_metainfo,
                                     boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                     signal_t *interruptor) THROWS_NOTHING;

    void do_get_a_metainfo(int i,
                           order_token_t order_token,
                           boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                           signal_t *interruptor,
                           region_map_t<protocol_t, version_range_t> *updatee,
                           mutex_t *updatee_mutex);

    void do_set_a_metainfo(int i,
                           const region_map_t<protocol_t, binary_blob_t> &new_metainfo,
                           order_token_t order_token,
                           boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                           signal_t *interruptor);


    // Used by the constructors.
    void initialize(store_view_t<protocol_t> **_store_views, const typename protocol_t::region_t &_region_mask) THROWS_NOTHING;

    // We _own_ these pointers and must delete them at destruction.
    scoped_array_t<store_view_t<protocol_t> *> store_views;

    typename protocol_t::region_t region;

    DISABLE_COPYING(multistore_ptr_t);
};




#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_

