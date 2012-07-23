#ifndef BACKFILL_PROGRESS_HPP_
#define BACKFILL_PROGRESS_HPP_

#include <vector>

#include "utils.hpp"

template <class> class scoped_ptr_t;

struct progress_completion_fraction_t {
    progress_completion_fraction_t(int _released, int _total) : estimate_of_released_nodes(_released), estimate_of_total_nodes(_total) {
        rassert(0 <= estimate_of_released_nodes && estimate_of_released_nodes <= estimate_of_total_nodes);
    }

    static progress_completion_fraction_t make_invalid() {
        return progress_completion_fraction_t();
    }

    int estimate_of_released_nodes;
    int estimate_of_total_nodes;

    bool invalid() const { return estimate_of_total_nodes == -1; }

private:
    // Used only by the static make_invalid() function.
    progress_completion_fraction_t() : estimate_of_released_nodes(-1), estimate_of_total_nodes(-1) { }
};

// TODO: Rename this to traversal_progress_t after it has been pushed
// and merged into rdb_protocol.
class traversal_progress_t : public home_thread_mixin_t {
public:
    traversal_progress_t() { }
    traversal_progress_t(int specified_home_thread) : home_thread_mixin_t(specified_home_thread) { }

    virtual progress_completion_fraction_t guess_completion() const = 0;

    // This actually gets used, by traversal_progress_combiner_t.
    virtual ~traversal_progress_t() { }
private:
    DISABLE_COPYING(traversal_progress_t);
};

class traversal_progress_combiner_t : public traversal_progress_t {
public:
    traversal_progress_combiner_t(int specified_home_thread) : traversal_progress_t(specified_home_thread), is_destructing(false) { }
    traversal_progress_combiner_t() : is_destructing(false) { }
    ~traversal_progress_combiner_t();

    // The constituent is welcome to have a different home thread.
    void add_constituent(scoped_ptr_t<traversal_progress_t> *constituent);
    progress_completion_fraction_t guess_completion() const;

private:
    // Used in a pmap by the destructor.
    void destroy_constituent(int i);

    // Used by guess_completion in a pmap call to go to the
    // constituent's home thread.
    void get_constituent_fraction(int i, std::vector<progress_completion_fraction_t> *outputs) const;

    // constituents _owns_ these pointers.
    std::vector<traversal_progress_t *> constituents;

    bool is_destructing;

    DISABLE_COPYING(traversal_progress_combiner_t);
};



#endif  // BACKFILL_PROGRESS_HPP_
