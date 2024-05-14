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

class ui {
   public:
    WINDOW* win = nullptr;

    short menu(const string& title, const vector<string>& choices) {
        noecho();
        curs_set(false);

        short choice = 0, input = 0, maxLen = 0;

        mvwprintw(win, 0, 0, "%s", title.c_str());
        for (int i = 1; i < choices.size(); i++) {
            short len = choices[i].length();
            if (len > maxLen) maxLen = len;
        }
        wattron(win, A_STANDOUT);
        mvwprintw(win, 1, 1, "%s%*c", choices[0].c_str(),
                  maxLen - choices[0].length() + 1, '\0');
        wattroff(win, A_STANDOUT);
        for (int i = 1; i < choices.size(); i++)
            mvwprintw(win, i + 1, 1, "%s", choices[i].c_str());
        do {
            switch (input) {
                case 'k':
                case KEY_UP:
                    if (choice > 0) {
                        mvwprintw(win, choice + 1, 1, "%s%*c",
                                  choices[choice].c_str(),
                                  maxLen - choices[choice].length() + 1, '\0');
                        --choice;
                    }

                    break;
                case 'j':
                case KEY_DOWN:
                    if (choice < choices.size() - 1) {
                        mvwprintw(win, choice + 1, 1, "%s%*c",
                                  choices[choice].c_str(),
                                  maxLen - choices[choice].length() + 1, '\0');
                        ++choice;
                    }
                    break;
                default:
                    break;
            }
            wattron(win, A_STANDOUT);
            mvwprintw(win, choice + 1, 1, "%s%*c", choices[choice].c_str(),
                      maxLen - choices[choice].length() + 1, '\0');
            wattroff(win, A_STANDOUT);
        } while ((input = wgetch(win)) != '\n');

        clear();
        echo();
        curs_set(true);
        return choice;
    }

    void progressBar(const long& prog, const long& total) {
        short y, x;
        getyx(win, y, x);
        const int len = 30;
        short percent = ceil(float(len) / total * prog);
        wchar_t str[len + 1];
        for (short i = 0; i < percent; i++) str[i] = L'\u2588';
        for (short i = percent; i < len; i++) str[i] = L'\u2592';
        str[len] = L'\0';
        mvwprintw(win, y, 1, "|%ls|", str);
        if (prog == total) wmove(win, y + 1, 1);
        wrefresh(win);
    }

    void notification(const string& title, const vector<string>& message) {
        noecho();
        curs_set(false);
        short maxWidth = 0, maxHeight = message.size(), y, x;
        for (const string& str : message) {
            short len = str.length();
            if (len > maxWidth) maxWidth = len;
        }
        short titleLen = title.length();
        if (titleLen > maxWidth) maxWidth = titleLen;
        maxWidth += 4;
        getmaxyx(stdscr, y, x);
        WINDOW* ntfwin =
            newwin(maxHeight + 2, maxWidth, y / 2 - (maxHeight + 2) / 2,
                   x / 2 - maxWidth / 2);
        box(ntfwin, 0, 0);
        mvwprintw(ntfwin, 0, maxWidth / 2 - titleLen / 2, "%s", title.c_str());
        for (short i = 0; i < maxHeight; i++)
            mvwprintw(ntfwin, i + 1, maxWidth / 2 - message[i].length() / 2,
                      "%s", message[i].c_str());
        wrefresh(ntfwin);
        wgetch(ntfwin);
        werase(ntfwin);
        wrefresh(ntfwin);
        delwin(ntfwin);
        echo();
        curs_set(true);
    }

    ui() {
        setlocale(LC_ALL, "");
        initscr();
        cbreak();
        short y, x;
        getmaxyx(stdscr, y, x);
        win = newwin(y, x, 0, 0);
        scrollok(win, true);
        keypad(win, true);
        box(win, 0, 0);
        refresh();
        wrefresh(win);
    }
    ~ui() {
        delwin(win);
        endwin();
    }
};

int main() {
    ui ui;
    // ui.menu("Kurwa:", {"sex", "boobies", "AAA"});
    ui.notification("KURWA", {"SEEEEEEEEEEEEEEEEEEX",
                              "SEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEX", "SEX"});
    int n = 29;
    for (int i = 0; i < 30; i++) {
        for (int j = 0; j <= n; j++) {
            this_thread::sleep_for(ch::milliseconds(10));
            ui.progressBar(j, n);
        }
    }
    getch();
}
