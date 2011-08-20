#include "progress/progress.hpp"

#include <sys/stat.h>

#include "utils.hpp"
#include <boost/bind.hpp>

progress_bar_t::progress_bar_t(std::string _activity, int _redraw_interval_ms = 100)
    : repeating_timer_t(redraw_interval_ms, boost::bind(&progress_bar_t::refresh, this)),
      activity(_activity), redraw_interval_ms(_redraw_interval_ms), start_time(get_ticks()),
      total_refreshes(0)
{ }

void progress_bar_t::refresh() {
    total_refreshes++;
    reset_bar();
    draw();
    reset_bar();
}

void progress_bar_t::reset_bar() {
    printf("\r");
    fflush(stdout);
}

/* progress should be in [0.0,1.0] */
void progress_bar_t::draw_bar(float progress,  int eta) {
    int percent_done = int(progress * 100);
    printf("%s: ", activity.c_str());
    printf("[");
    for (int i = 1; i < 49; i++) {
        if (i % 5 == 0) printf("%d", 2 * i);
        else if (i == percent_done / 2) printf(">");
        else if (i < percent_done / 2) printf("=");
        else printf(" ");
    }
    printf("] ");

    if (eta == -1 && progress > 0) {
        //Do automatic linear interpolation for eta
        eta = int(((1.0f / progress) - 1) * ticks_to_secs(get_ticks() - start_time));
    }

    if (eta == -1) printf("ETA: -");
    else printf("ETA: %01d:%02d:%02d", (eta / 3600), (eta / 60) % 60, eta % 60);

    printf("                 "); //make sure we don't leave an characters behind
    fflush(stdout);
}


counter_progress_bar_t::counter_progress_bar_t(std::string activity, int _expected_count, int redraw_interval_ms)
    : progress_bar_t(activity, redraw_interval_ms), count(0), expected_count(_expected_count) { }

void counter_progress_bar_t::draw() {
    progress_bar_t::draw_bar(float(count) / float(expected_count), -1);
}

void counter_progress_bar_t::operator++(int) {
    count++;
}

file_progress_bar_t::file_progress_bar_t(std::string activity, FILE *_file, int redraw_interval_ms)
    : progress_bar_t(activity, redraw_interval_ms), file(_file) { 
    struct stat file_stats;
    fstat(fileno(file), &file_stats);
    file_size = file_stats.st_size;
}

void file_progress_bar_t::draw() {
    progress_bar_t::draw_bar(float(ftell(file)) / float(file_size), -1);
}
