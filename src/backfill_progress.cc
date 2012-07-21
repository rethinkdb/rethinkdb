#include "backfill_progress.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/pmap.hpp"

traversal_progress_combiner_t::~traversal_progress_combiner_t() {
    is_destructing = true;
    pmap(constituents.size(), boost::bind(&traversal_progress_combiner_t::destroy_constituent, this, _1));
}

void traversal_progress_combiner_t::destroy_constituent(int i) {
    on_thread_t th(constituents[i]->home_thread());
    delete constituents[i];
}

void traversal_progress_combiner_t::add_constituent(traversal_progress_t *c) {
    assert_thread();
    guarantee(!is_destructing);
    constituents.push_back(c);
}

void traversal_progress_combiner_t::get_constituent_fraction(int i, std::vector<progress_completion_fraction_t> *outputs) const {
    guarantee(size_t(i) < constituents.size());
    rassert(size_t(i) < outputs->size());
    guarantee(!is_destructing);

    on_thread_t th(constituents[i]->home_thread());
    (*outputs)[i] = constituents[i]->guess_completion();
}

progress_completion_fraction_t traversal_progress_combiner_t::guess_completion() const {
    assert_thread();
    guarantee(!is_destructing);

    int released = 0, total = 0;

    std::vector<progress_completion_fraction_t> fractions(constituents.size(), progress_completion_fraction_t::make_invalid());
    pmap(fractions.size(), boost::bind(&traversal_progress_combiner_t::get_constituent_fraction, this, _1, &fractions));

    for (std::vector<progress_completion_fraction_t>::const_iterator it = fractions.begin();
         it != fractions.end();
         ++it) {
        if (it->invalid()) {
            return progress_completion_fraction_t::make_invalid();
        }

        released += it->estimate_of_released_nodes;
        total += it->estimate_of_total_nodes;
    }

    return progress_completion_fraction_t(released, total);
}
