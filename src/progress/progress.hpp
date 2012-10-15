#ifndef PROGRESS_PROGRESS_HPP_
#define PROGRESS_PROGRESS_HPP_

#include <stdio.h>

#include <string>

#include "arch/timing.hpp"

struct progress_bar_draw_callback_t {
    virtual void draw() = 0;
protected:
    virtual ~progress_bar_draw_callback_t() { }
};

class progress_bar_t {
public:
    explicit progress_bar_t(const std::string&);
    virtual ~progress_bar_t();

    void refresh(progress_bar_draw_callback_t *);
    void reset_bar();
    void draw_bar(double progress, int eta = -1);

    // repeating_timer_t callback
    void on_ring();

private:
    // The activity this progress bar represents
    std::string activity;

    // The redraw time interval.
    int redraw_interval_ms;

    // The time when we started.
    ticks_t start_time;

    // The number of refreshes we have begun so far.
    int total_refreshes;

    DISABLE_COPYING(progress_bar_t);
};

class counter_progress_bar_t : public repeating_timer_callback_t, public progress_bar_draw_callback_t {
public:
    counter_progress_bar_t(const std::string&, int, int redraw_interval_ms = 100);
    virtual ~counter_progress_bar_t() { }

    void operator++();
private:
    void draw();
    void on_ring();

    int count, expected_count;

    progress_bar_t progress_bar;

    repeating_timer_t timer;

    DISABLE_COPYING(counter_progress_bar_t);
};

// File progress bar watches as you use a file to see how fast you're
// using it, it does not ever write or read from the file.
class file_progress_bar_t : public repeating_timer_callback_t, public progress_bar_draw_callback_t {
public:
    file_progress_bar_t(const std::string&, FILE *, int redraw_interval_ms = 100);
    virtual ~file_progress_bar_t() { }

private:
    void draw();
    void on_ring();

    FILE *file;
    int file_size;

    progress_bar_t progress_bar;

    repeating_timer_t timer;
};

#endif // PROGRESS_PROGRESS_HPP_
