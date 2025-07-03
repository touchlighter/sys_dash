#include "sysinfo.h"
#include <ncurses.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>

#define REFRESH_INTERVAL_MS 1000

std::string format_time(float seconds) {
    int h = seconds / 3600;
    int m = ((int)seconds % 3600) / 60;
    int s = (int)seconds % 60;
    char buf[64];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    return std::string(buf);
}

void print_bar(WINDOW* win, int y, int x, float percent, int width = 30) {
    int filled = (int)(percent / 100.0f * width);
    mvwprintw(win, y, x, "[");
    for (int i = 0; i < width; ++i)
        waddch(win, i < filled ? '=' : ' ');
    wprintw(win, "] %.1f%%", percent);
}

void draw_dashboard(WINDOW* win, const SysStats& s) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 1, 2, "SysInfo Dashboard - Intel iMac 2017");

    mvwprintw(win, 3, 2, "CPU: %s", s.cpu_brand.c_str());
    mvwprintw(win, 4, 4, "Usage: ");
    print_bar(win, 4, 12, s.cpu_usage_percent);

    mvwprintw(win, 6, 2, "Memory:");
    mvwprintw(win, 7, 4, "%.2f GB used / %.2f GB total", s.memory_used_gb, s.memory_total_gb);
    print_bar(win, 8, 4, s.memory_percent);

    mvwprintw(win, 10, 2, "Disk:");
    mvwprintw(win, 11, 4, "%.2f GB used / %.2f GB total", s.disk_used_gb, s.disk_total_gb);
    print_bar(win, 12, 4, s.disk_percent);

    mvwprintw(win, 14, 2, "Uptime: %s", format_time(s.uptime_seconds).c_str());

    mvwprintw(win, 16, 2, "Battery: %s (%d%%)", s.battery_status.c_str(), s.battery_percent);

    mvwprintw(win, 18, 2, "Press Q to quit.");
    wrefresh(win);
}

int main() {
    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    curs_set(0);

    sysinfo::initialize();

    WINDOW* win = newwin(25, 85, 0, 0);

    bool running = true;
    while (running) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = false;
            break;
        }

        auto stats = sysinfo::collect();
        draw_dashboard(win, stats);
        std::this_thread::sleep_for(std::chrono::milliseconds(REFRESH_INTERVAL_MS));
    }

    sysinfo::shutdown();
    endwin();
    return 0;
}

