#ifndef CLUSTERING_ADMINISTRATION_ISSUE_SUBSCRIPTION_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUE_SUBSCRIPTION_HPP_

#include "concurrency/signal.hpp"
#include "clustering/administration/issues/local.hpp"

class issue_subscription_t : public signal_t::subscription_t {
public:
    explicit issue_subscription_t(scoped_ptr_t<local_issue_tracker_t::entry_t> *_entry) :
        entry(_entry) { }

    void run() {
        entry->reset();
    }

private:
    scoped_ptr_t<local_issue_tracker_t::entry_t> *entry;
    DISABLE_COPYING(issue_subscription_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUE_SUBSCRIPTION_HPP_ */
