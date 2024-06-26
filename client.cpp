#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <vector>

#include "sr.hpp"

using std::string, std::vector, nlohmann::json, std::cerr, std::endl,
    std::runtime_error;
namespace fs = std::filesystem;
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

    string getinput(const string& title, const short& minLength) {
        char buffer[1024];
        int y, x;
        getyx(stdscr, y, x);
        printw("%s", title.c_str());
        while (true) {
            move(y, title.length());
            clrtoeol();
            getnstr(buffer, sizeof(buffer) - 1);
            if (strlen(buffer) < minLength)
                notification(
                    "Invalid Input",
                    {"Input length should be ",
                     "at least " + to_string(minLength) + " characters."});
            else
                break;
        }
        return buffer;
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

class clsClient {
   public:
    clsSock sock;
    clsUi ui;

    void reg() {
        char buffer[1024];
        bool ok;
        printw("Sign Up");
        do {
            move(1, 0);
            clrtobot();
            sock.send(ui.getinput("Enter Username: ", 3));
            if ((ok = sock.recv() != "ok"))
                ui.notification("Alert", {"This username is already in use."});
        } while (ok);
        sock.send(ui.getinput("Enter Password: ", 3));
        ui.notification("Success!", {"You have registered a new account."});
        clear();
        refresh();
    }

    void log() {
        char buffer[1024];
        bool ok;
        printw("Sign In");
        do {
            move(1, 0);
            clrtobot();
            sock.send(ui.getinput("Enter Username: ", 3));
            sock.send(ui.getinput("Enter Password: ", 3));
            if ((ok = sock.recv() != "ok"))
                ui.notification("Alert",
                                {"The password or username is incorrect."});
        } while (ok);
        ui.notification("Success!", {"You signed in to your account."});
        clear();
        refresh();
    }

    clsClient(SSL* ssl, clsUi& ui) : sock(ssl), ui(ui) {}
};

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
class project {
   private:
    json genJson(string path) {
        json res;
        for (const fs::directory_entry& entry : fs::directory_iterator(path)) {
            if (entry.is_directory())
                res[entry.path().filename().string()] =
                    genJson(entry.path().string());
            else
                res[entry.path().filename().string()] =
                    ch::duration_cast<ch::seconds>(
                        fs::last_write_time(entry).time_since_epoch())
                        .count();
        }
        return res;
    }
    void compJson(json& oldjs, json& newjs, string path) {
        for (json::iterator it = oldjs.begin(); it != oldjs.end(); ++it) {
            if (newjs.contains(it.key())) {
                if (it.value().is_number() && newjs[it.key()].is_object()) {
                    printw(
                        "File ./%s%s was deleted.\nFolder ./%s%s was "
                        "created.\n",
                        path.c_str(), it.key().c_str(), path.c_str(),
                        it.key().c_str());
                    owner.sock.send("removeFile " + path + it.key());
                    owner.sock.send("createDir " + path + it.key());
                    json temp;
                    compJson(temp, newjs[it.key()], path + it.key() + '/');
                } else if (it.value().is_object() &&
                           newjs[it.key()].is_number()) {
                    printw(
                        "Folder ./%s%s was deleted.\nFile ./%s%s was "
                        "created.\n",
                        path.c_str(), it.key().c_str(), path.c_str(),
                        it.key().c_str());
                    owner.sock.send("removeDir " + path + it.key());
                    owner.sock.send("createFile " + path + it.key());
                    if (owner.sock.recv() == "ok")
                        owner.sock.sendFile(prjPath + path + it.key(),
                                            &progressBar);
                } else if (newjs[it.key()].is_object()) {
                    compJson(it.value(), newjs[it.key()],
                             path + it.key() + '/');
                } else if (it.value() != newjs[it.key()]) {
                    printw("File ./%s%s was changed.\n", path.c_str(),
                           it.key().c_str());
                    owner.sock.send("createFile " + path + it.key());
                    if (owner.sock.recv() == "ok")
                        owner.sock.sendFile(prjPath + path + it.key(),
                                            &progressBar);
                }
            } else {
                if (it.value().is_object() || it.value().is_null()) {
                    printw("Folder ./%s%s was deleted.\n", path.c_str(),
                           it.key().c_str());
                    owner.sock.send("removeDir " + path + it.key());
                } else {
                    printw("File ./%s%s was deleted.\n", path.c_str(),
                           it.key().c_str());
                    owner.sock.send("removeFile " + path + it.key());
                }
            }
        }
        for (json::iterator it = newjs.begin(); it != newjs.end(); ++it) {
            if (!oldjs.contains(it.key())) {
                if (it.value().is_object() || it.value().is_null()) {
                    printw("Folder ./%s%s was created.\n", path.c_str(),
                           it.key().c_str());
                    owner.sock.send("createDir " + path + it.key());
                    json temp;
                    compJson(temp, it.value(), path + it.key() + '/');
                } else {
                    printw("File ./%s%s was created.\n", path.c_str(),
                           it.key().c_str());
                    owner.sock.send("createFile " + path + it.key());
                    if (owner.sock.recv() == "ok")
                        owner.sock.sendFile(prjPath + path + it.key(),
                                            &progressBar);
                }
            }
        }
    }

   public:
    clsClient owner;
    string prjName;
    string prjPath;

    void set() {
        char buffer[1024];
        bool ok;
        do {
            prjPath = owner.ui.getinput("Path to your project: ", 1);
            if ((ok = !fs::exists(prjPath) && !fs::is_directory(prjPath)))
                owner.ui.notification("Alert", {"Path is invalid."});
        } while (ok);
        prjPath += '/';
        do {
            prjName = owner.ui.getinput("Name of your project: ", 3);
            owner.sock.send(prjName);
            if ((ok = owner.sock.recv() != "ok"))
                owner.ui.notification("Alert", {"Project name is invalid."});
        } while (ok);
        clear();
    }
    void open() {
        string command;
        while (true) {
            if ((command = owner.ui.getinput("> ", 3)) == "push") {
                owner.sock.send("push");
                json curr = json::parse(owner.sock.recv());
                json check = genJson(prjPath);
                compJson(curr, check, "");
                owner.sock.send(check.dump());
            } else if (command == "back") {
                owner.sock.send("back");
                break;
            }
        }
        clear();
    }
    void download() {
        try {
            string command;
            while ((command = owner.sock.recv()) != "done") {
                string action = command.substr(0, command.find(' '));
                string path = command.substr(command.find(' ') + 1);
                if (action == "createDir") {
                    printw("Folder %s was created\n", path.c_str());
                    fs::create_directory(prjPath + path);
                } else if (action == "createFile") {
                    printw("File %s was created\n", path.c_str());
                    owner.sock.recvFile(prjPath + path, &progressBar);
                }
            }
        } catch (const runtime_error& e) {
            cerr << e.what() << endl;
        }
    }

    project(clsClient& clsClient) : owner(clsClient) {}
};

int main(int argc, char* argv[]) {
    string path = fs::canonical(argv[0]).parent_path().string() + '/';

    SSL_library_init();
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());

    SSL_CTX_load_verify_locations(ctx, (path + "cert.pem").c_str(), NULL);

    clsUi ui;
    clsClient client(SSL_new(ctx), ui);

    switch (ui.menu("Choose one option:", {"Sign In", "Sign Up"})) {
        case 0:
            client.sock.send("signIn");
            client.log();
            break;
        case 1:
            client.sock.send("signUp");
            client.reg();
            client.log();
            break;
    }
    while (true) {
        project project(client);
        switch (ui.menu("Choose one option:",
                        {"Create new project", "Open existing project",
                         "Download project from server", "Exit"})) {
            case 0:
                client.sock.send("createPrj");
                project.set();
                project.open();
                break;
            case 1:
                client.sock.send("openPrj");
                project.set();
                project.open();
                break;
            case 2:
                client.sock.send("downloadPrj");
                project.set();
                project.download();
                project.open();
                break;
            case 3:
                client.sock.send("quit");
                SSL_CTX_free(ctx);
                client.sock.close();
                return 0;
        }
    }
}
