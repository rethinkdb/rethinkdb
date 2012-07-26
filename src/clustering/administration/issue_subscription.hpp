#ifndef __CLUSTERING_ADMINISTRATION_ISSUE_SUBSCRIPTION__
#define __CLUSTERING_ADMINISTRATION_ISSUE_SUBSCRIPTION__

#include "concurrency/signal.hpp"
#include "clustering/administration/issues/local.hpp"

class issue_subscription_t : public signal_t::subscription_t {
public:
    issue_subscription_t(scoped_ptr_t<local_issue_tracker_t::entry_t> *_entry) :
        entry(_entry) { }

    void run() {
        entry->reset();
    }

private:
    scoped_ptr_t<local_issue_tracker_t::entry_t> *entry;
};

#endif /*__CLUSTERING_ADMINISTRATION_ISSUE_SUBSCRIPTION__*/
