#ifndef UNITTEST_SIMPLE_MAILBOX_CLUSTER_HPP_
#define UNITTEST_SIMPLE_MAILBOX_CLUSTER_HPP_

#include <memory>

#include "containers/scoped.hpp"

class connectivity_cluster_t;
class mailbox_manager_t;

namespace unittest {

class simple_mailbox_cluster_t {
public:
    simple_mailbox_cluster_t();
    ~simple_mailbox_cluster_t();
    connectivity_cluster_t *get_connectivity_cluster();
    mailbox_manager_t *get_mailbox_manager();
    void connect(simple_mailbox_cluster_t *other);
    void disconnect(simple_mailbox_cluster_t *other);
private:
    struct simple_mailbox_cluster_state;
    scoped_ptr_t<simple_mailbox_cluster_state> state;
};

}  // namespace unittest

#endif  // UNITTEST_SIMPLE_MAILBOX_CLUSTER_HPP_
