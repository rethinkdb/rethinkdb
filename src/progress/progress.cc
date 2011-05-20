#include "progress.hpp"

progress_bar_t::progress_bar_t(std::string activity, int redraw_interval_ms = 100) 
    : repeating_timer_t(redraw_interval_ms, 
      boost::bind(&progress_bar_t::refresh, this)), activity(activity), 
      redraw_interval_ms(redraw_interval_ms), total_refreshes(0)
{ }

void progress_bar_t::refresh() {
    total_refreshes++;
    reset_bar();
    draw();
    reset_bar();
}

void progress_bar_t::reset_bar() { 
    printf("\r"); 
    fflush(stdin);
}

void progress_bar_t::draw_bar(int percent_done,  int eta) {
    printf("%s: ", activity.c_str());
    printf("[");
    for (int i = 1; i < 49; i++) {
        if (i % 5 == 0) printf("%d", 2 * i);
        else if (i == percent_done / 2) printf(">");
        else if (i < percent_done / 2) printf("=");
        else printf(" ");
    }
    printf("] ");

    if (eta == -1) {
        //Do automatic linear interpolation for eta
        eta = ((100.0f / float(percent_done)) - 1) * total_refreshes * redraw_interval_ms /* ms */ * 0.001f /* ms/s */;
    }
        
    printf("ETA: %d:%02d", eta / 60, eta % 60);
    fflush(stdin);
}


counter_progress_bar_t::counter_progress_bar_t(std::string activity, int expected_count, int redraw_interval_ms = 100) 
    : progress_bar_t(activity, redraw_interval_ms), count(0), expected_count(expected_count)
{ }

void counter_progress_bar_t::draw() {
    progress_bar_t::draw_bar(int(float(count * 100) / float(expected_count)), -1);
}

file_progress_bar_t::file_progress_bar_t(std::string activity, FILE *file, int redraw_interval_ms)
    : progress_bar_t(activity, redraw_interval_ms), file(file)
{ 
    struct stat file_stats;
    fstat(fileno(file), &file_stats);
    file_size = file_stats.st_size;
}

void file_progress_bar_t::draw() {
    progress_bar_t::draw_bar(int(float(ftell(file) * 100) / float(file_size)), -1);
}
