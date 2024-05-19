#include <ncurses.h>

#include <chrono>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace std;
namespace ch = std::chrono;

class clsUi {
   public:
    short maxx, maxy;

    short menu(const string& title, const vector<string>& choices) {
        noecho();
        curs_set(false);
        short choice = 0, input = 0, maxWidth = 0, maxHeight = choices.size(),
              minWidth = 15;
        for (const string& str : choices) {
            short len = str.length();
            if (len > maxWidth) maxWidth = len;
        }
        short titleLen = title.length();
        if (titleLen > maxWidth) maxWidth = titleLen;
        if (minWidth > maxWidth) maxWidth = minWidth;
        WINDOW* win =
            newwin(maxHeight + 2, maxWidth + 2, maxy / 2 - (maxHeight + 2) / 2,
                   maxx / 2 - (maxWidth + 2) / 2);
        keypad(win, true);
        box(win, 0, 0);
        mvwprintw(win, 0, 1, "%s", title.c_str());
        for (int i = 1; i < choices.size(); i++)
            mvwprintw(win, i + 1, 1, "%s", choices[i].c_str());
        do {
            switch (input) {
                case 'k':
                case KEY_UP:
                    if (choice > 0) {
                        mvwprintw(win, choice + 1, 1, "%s%*c",
                                  choices[choice].c_str(),
                                  maxWidth - choices[choice].length() + 1,
                                  '\0');
                        --choice;
                    }
                    break;
                case 'j':
                case KEY_DOWN:
                    if (choice < choices.size() - 1) {
                        mvwprintw(win, choice + 1, 1, "%s%*c",
                                  choices[choice].c_str(),
                                  maxWidth - choices[choice].length() + 1,
                                  '\0');
                        ++choice;
                    }
                    break;
                default:
                    break;
            }
            wattron(win, A_STANDOUT);
            mvwprintw(win, choice + 1, 1, "%s%*c", choices[choice].c_str(),
                      maxWidth - choices[choice].length() + 1, '\0');
            wattroff(win, A_STANDOUT);
        } while ((input = wgetch(win)) != '\n');
        werase(win);
        wrefresh(win);
        delwin(win);
        echo();
        curs_set(true);
        return choice;
    }

    void progressBar(const long& prog, const long& total) {
        short y, x;
        getyx(stdscr, y, x);
        const int len = 30;
        short percent = ceil(float(len) / total * prog);
        wchar_t str[len + 1];
        for (short i = 0; i < percent; i++) str[i] = L'\u2588';
        for (short i = percent; i < len; i++) str[i] = L'\u2592';
        str[len] = L'\0';
        mvprintw(y, 0, "|%ls|", str);
        if (prog == total) move(y + 1, 0);
        refresh();
    }

    void notification(const string& title, const vector<string>& message) {
        noecho();
        curs_set(false);
        short maxWidth = 0, maxHeight = message.size(),
              titleLen = title.length();
        for (const string& str : message) {
            short len = str.length();
            if (len > maxWidth) maxWidth = len;
        }
        if (titleLen > maxWidth) maxWidth = titleLen;
        maxWidth += 4;
        WINDOW* win =
            newwin(maxHeight + 2, maxWidth, maxy / 2 - (maxHeight + 2) / 2,
                   maxx / 2 - maxWidth / 2);
        box(win, 0, 0);
        mvwprintw(win, 0, maxWidth / 2 - titleLen / 2, "%s", title.c_str());
        for (short i = 0; i < maxHeight; i++)
            mvwprintw(win, i + 1, maxWidth / 2 - message[i].length() / 2, "%s",
                      message[i].c_str());
        wrefresh(win);

        wgetch(win);
        werase(win);
        wrefresh(win);
        delwin(win);

        echo();
        curs_set(true);
    }

    clsUi() {
        setlocale(LC_ALL, "");
        initscr();
        cbreak();
        scrollok(stdscr, true);
        keypad(stdscr, true);
        getmaxyx(stdscr, maxy, maxx);
    }
    ~clsUi() { endwin(); }
};

int main() {
    clsUi ui;
    ui.menu("KURWA:", {"Boobies", "SEEEX", "DICK"});
    int n = 29;
    auto callback = [&ui](const long& prog, const long& total) {
        ui.progressBar(prog, total);
    };
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j <= n; j++) {
            this_thread::sleep_for(ch::milliseconds(1));
            callback(j, n);
        }
    }
    ui.notification("SUCKSEX!!!", {"Go Fuck Yourself :3"});
}
