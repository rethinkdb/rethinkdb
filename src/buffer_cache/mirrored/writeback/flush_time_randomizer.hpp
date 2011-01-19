#ifndef __BUFFER_CACHE_MIRRORED_WRITEBACK_FLUSHTIMERANDOMIZER_HPP__
#define __BUFFER_CACHE_MIRRORED_WRITEBACK_FLUSHTIMERANDOMIZER_HPP__

#include "server/cmd_args.hpp"

// Provides flush intervals for us to use.
class flush_time_randomizer_t {
public:
    explicit flush_time_randomizer_t(int flush_timer_ms_);

    // Creates a probability distribution to twiggle with flush timing
    // intervals, to combat any long-term tendency of flushes to get
    // grouped together.  Returns a value in (0, flush_timer_ms], but
    // usually returns flush_timer_ms.
    int next_time_interval();

    // returns flush_timer_ms == NEVER_FLUSH, meaning we never flush.
    inline bool is_never_flush() const { return flush_timer_ms == NEVER_FLUSH; }

    // returns flush_timer_ms == 0, meaning we flush immediately.
    inline bool is_zero() const { return flush_timer_ms == 0; }
private:
    const int flush_timer_ms;
    const int first_time_interval;
    bool done_first_time_interval;

};






#endif  // __BUFFER_CACHE_MIRRORED_WRITEBACK_FLUSHTIMERANDOMIZER_HPP__
