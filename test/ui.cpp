#include <ncurses.h>

#include <clocale>
#include <string>
#include <vector>

using namespace std;

class ui {
   public:
    WINDOW* win = nullptr;

    short drawmenu(const string& title, const vector<string>& choices) {
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

        echo();
        curs_set(true);
        return choice;
    }
    ui() {
        setlocale(LC_ALL, "");
        initscr();
        cbreak();
        int y, x;
        getyx(stdscr, y, x);
        win = newwin(x, y, 0, 0);
        scrollok(win, true);
        keypad(win, true);
        box(win, 0, 0);
        refresh();
        wrefresh(win);
    }
    ~ui() { endwin(); }
};

int main() {
    ui ui;
    ui.drawmenu("Kurwa:", {"sex", "boobies", "AAA"});
    getch();
}
