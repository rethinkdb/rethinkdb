#ifndef MOCK_TEST_CLUSTER_GROUP_HPP_
#define MOCK_TEST_CLUSTER_GROUP_HPP_

#include <map>
#include <string>
#include <set>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "containers/cow_ptr.hpp"
#include "containers/scoped.hpp"
#include "clustering/reactor/directory_echo.hpp"


template <class> class blueprint_t;
template <class> class cluster_namespace_interface_t;
class io_backender_t;
template <class> class multistore_ptr_t;
template <class> class reactor_business_card_t;
class peer_id_t;

namespace mock {

class temp_file_t;

template <class> class reactor_test_cluster_t;
template <class> class test_reactor_t;

template<class protocol_t>
class test_cluster_directory_t {
public:
    boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > reactor_directory;

    RDB_MAKE_ME_SERIALIZABLE_1(reactor_directory);
};


template<class protocol_t>
class test_cluster_group_t {
public:
    boost::ptr_vector<temp_file_t> files;
    scoped_ptr_t<io_backender_t> io_backender;
    boost::ptr_vector<typename protocol_t::store_t> stores;
    boost::ptr_vector<multistore_ptr_t<protocol_t> > svses;
    boost::ptr_vector<reactor_test_cluster_t<protocol_t> > test_clusters;

    boost::ptr_vector<test_reactor_t<protocol_t> > test_reactors;

    std::map<std::string, std::string> inserter_state;

    typename protocol_t::context_t ctx;

    explicit test_cluster_group_t(int n_machines);
    ~test_cluster_group_t();

    void construct_all_reactors(const blueprint_t<protocol_t> &bp);

    peer_id_t get_peer_id(unsigned i);

    blueprint_t<protocol_t> compile_blueprint(const std::string& bp);

    void set_all_blueprints(const blueprint_t<protocol_t> &bp);

    static std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > extract_reactor_business_cards_no_optional(
            const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &input);

    void make_namespace_interface(int i, scoped_ptr_t<cluster_namespace_interface_t<protocol_t> > *out);

    void run_queries();

    static std::map<peer_id_t, boost::optional<cow_ptr_t<reactor_business_card_t<protocol_t> > > > extract_reactor_business_cards(
            const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &input);

    void wait_until_blueprint_is_satisfied(const blueprint_t<protocol_t> &bp);

    void wait_until_blueprint_is_satisfied(const std::string& bp);
};

}   /* namespace mock */

#endif  // MOCK_TEST_CLUSTER_GROUP_HPP_
