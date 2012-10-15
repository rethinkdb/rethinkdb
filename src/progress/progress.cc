#include "progress/progress.hpp"

#include <sys/stat.h>


// TODO: We're using printf??  printf can block the thread.

progress_bar_t::progress_bar_t(const std::string& _activity)
    : activity(_activity), start_time(get_ticks()), total_refreshes(0) { }

progress_bar_t::~progress_bar_t() {
    if (total_refreshes > 0)
        printf("\n");
}

void progress_bar_t::refresh(progress_bar_draw_callback_t *cb) {
    total_refreshes++;
    reset_bar();
    cb->draw();
    reset_bar();
}

void progress_bar_t::reset_bar() {
    printf("\r");
    fflush(stdout);
}

/* progress should be in [0.0,1.0] */
void progress_bar_t::draw_bar(double progress, int eta) {
    int percent_done = static_cast<int>(progress * 100);
    printf("%s: ", activity.c_str());
    printf("[");
    for (int i = 1; i < 49; i++) {
        if (i % 5 == 0) {
            printf("%d", 2 * i);
        } else if (i == percent_done / 2) {
            printf(">");
        } else if (i < percent_done / 2) {
            printf("=");
        } else {
            printf(" ");
        }
    }
    printf("] ");

    if (eta == -1 && progress > 0) {
        //Do automatic linear interpolation for eta
        eta = static_cast<int>(((1.0 / progress) - 1) * ticks_to_secs(get_ticks() - start_time));
    }

    if (eta == -1) {
        printf("ETA: -");
    } else {
        printf("ETA: %01d:%02d:%02d", (eta / 3600), (eta / 60) % 60, eta % 60);
    }

    printf("                 "); //make sure we don't leave an characters behind
    fflush(stdout);
}


counter_progress_bar_t::counter_progress_bar_t(const std::string& activity, int _expected_count, int redraw_interval_ms)
    : count(0), expected_count(_expected_count), progress_bar(activity), timer(redraw_interval_ms, this) { }

void counter_progress_bar_t::draw() {
    progress_bar.draw_bar(static_cast<double>(count) / static_cast<double>(expected_count), -1);
}

void counter_progress_bar_t::on_ring() {
    progress_bar.refresh(this);
}


void counter_progress_bar_t::operator++() {
    count++;
}

file_progress_bar_t::file_progress_bar_t(const std::string& activity, FILE *_file, int redraw_interval_ms)
    : file(_file), progress_bar(activity), timer(redraw_interval_ms, this) {
    struct stat file_stats;
    fstat(fileno(file), &file_stats);
    file_size = file_stats.st_size;
}

void file_progress_bar_t::draw() {
    progress_bar.draw_bar(static_cast<double>(ftell(file)) / static_cast<double>(file_size), -1);
}

void file_progress_bar_t::on_ring() {
    progress_bar.refresh(this);
}
