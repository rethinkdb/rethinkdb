#ifndef PROGRESS_HPP_
#define PROGRESS_HPP_

#include <stdio.h>

#include <string>

#include "arch/timing.hpp"

class progress_bar_t : public repeating_timer_callback_t {
public:
    progress_bar_t(const std::string&, int);
    virtual ~progress_bar_t();

    void refresh();
    virtual void draw() = 0;
    void reset_bar();
    void draw_bar(float, int eta = -1);

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

    // The timer.  It is important that we destroy this before doing
    // anything blocking in the destructor.
    repeating_timer_t timer;

    DISABLE_COPYING(progress_bar_t);
};

class counter_progress_bar_t : public progress_bar_t {
public:
    counter_progress_bar_t(const std::string&, int, int redraw_interval_ms = 100);
    void operator++();
private:
    void draw();

    int count, expected_count;
};

// File progress bar watches as you use a file to see how fast you're
// using it, it does not ever write or read from the file.
class file_progress_bar_t : public progress_bar_t {
public:
    file_progress_bar_t(const std::string&, FILE *, int redraw_interval_ms = 100);
private:
    void draw();

    FILE *file;
    int file_size;
};

#endif //PROGRESS_HPP_
