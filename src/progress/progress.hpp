#ifndef __PROGRESS_HPP__
#define __PROGRESS_HPP__
#include <stdio.h>
#include <string>
#include "arch/timing.hpp"

class progress_bar_t : repeating_timer_t {
private:
    std::string activity; //the activity this progress bar represents
    int redraw_interval_ms;
    ticks_t start_time;
private:
    int total_refreshes;
public:
    progress_bar_t(std::string, int);
    virtual ~progress_bar_t() { 
        if (total_refreshes > 0)
            printf("\n"); 
    }

    void refresh();
    virtual void draw() = 0;
    void reset_bar();
    void draw_bar(float, int eta = -1); 
};

class counter_progress_bar_t : public progress_bar_t {
private:
    int count, expected_count;
public:
    counter_progress_bar_t(std::string, int, int redraw_interval_ms = 100);
    void draw();
    void operator++(int);
};

//file progress bar watches as you use a file to see how fast you're using it,
//it does not ever write or read from the file 
class file_progress_bar_t : public progress_bar_t {
private:
    FILE *file;
    int file_size;
public:
    file_progress_bar_t(std::string, FILE *, int redraw_interval_ms = 100);
    void draw();
};

#endif //__PROGRESS_HPP__
