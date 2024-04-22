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

using std::string, std::vector, nlohmann::json;
namespace fs = std::filesystem;
namespace ch = std::chrono;

char drawUI(string title, vector<string> arr) {
    noecho();
    char choice = 0, ch;
    bool kurwa = true;
    while (kurwa) {
        printw("%s\n", title.c_str());
        for (int i = 0; i < arr.size(); i++)
            printw("%c %s\n", i == choice ? '>' : ' ', arr[i].c_str());
        switch (ch = getch()) {
            case 'k':
            case KEY_UP:
                if (choice > 0) choice--;
                break;
            case 'j':
            case KEY_DOWN:
                if (choice < arr.size() - 1) choice++;
                break;
            case '\n':
                kurwa = false;
                break;
        }
        clear();
    }
    echo();
    return choice;
}
void progressBar(const long &prog, const long &total) {
    int y, x;
    getyx(stdscr, y, x);
    const int len = 30;
    int percent = ceil(float(len) / total * prog);
    wchar_t str[len + 1];
    for (int i = 0; i < percent; i++) str[i] = L'\u2588';
    for (int i = percent; i < len; i++) str[i] = L'\u2592';
    str[len] = L'\0';
    mvprintw(y, 0, "|%ls|", str);
    if (prog == total) addch('\n');
    refresh();
}

class client {
   public:
    clsSock sock;

    void reg() {
        char buffer[1024];
        do {
            clear();
            printw("Sign Up\nEnter Username: ");
            getstr(buffer);
            sock.send(buffer);
        } while (sock.recv() != "ok");
        printw("Enter Password: ");
        getstr(buffer);
        sock.send(buffer);
        clear();
    }

    void log() {
        char buffer[1024];
        do {
            printw("Sign In\nEnter Username: ");
            getstr(buffer);
            sock.send(buffer);
            printw("Enter Password: ");
            getstr(buffer);
            sock.send(buffer);
            clear();
        } while (sock.recv() != "ok");
    }
    client(SSL *ssl) : sock(ssl) {}
};

class project {
   private:
    json genJson(string path) {
        json res;
        for (const fs::directory_entry &entry : fs::directory_iterator(path)) {
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

    void compJson(json &oldjs, json &newjs, string path) {
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
    client owner;
    string prjName;
    string prjPath;

    void set() {
        char buffer[1024];
        do {
            printw("Path to your project: ");
            getstr(buffer);
            prjPath = buffer;
        } while (!fs::exists(prjPath) && !fs::is_directory(prjPath));
        //  prjPath = "/home/jomm/Documents/kurwa/client/test/";
        do {
            printw("Name of your project: ");
            getstr(buffer);
            prjName = buffer;
            owner.sock.send(prjName);
        } while (owner.sock.recv() != "ok");
        clear();
    }
    void open() {
        char buffer[20];
        while (true) {
            printw("> ");
            getstr(buffer);
            if (!strcmp(buffer, "push")) {
                owner.sock.send("push");
                json curr = json::parse(owner.sock.recv());
                json check = genJson(prjPath);
                compJson(curr, check, "");
                owner.sock.send(check.dump());
            } else if (!strcmp(buffer, "quit")) {
                owner.sock.send("quit");
                break;
            }
        }
    }
    void download() {
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
    }

    project(client &x) : owner(x) {}
};

int main(int argc, char *argv[]) {
    string path = fs::canonical(argv[0]).parent_path().string() + '/';

    setlocale(LC_ALL, "");
    initscr();
    scrollok(stdscr, TRUE);
    keypad(stdscr, TRUE);

    SSL_library_init();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

    SSL_CTX_load_verify_locations(ctx, (path + "cert.pem").c_str(), NULL);

    client client(SSL_new(ctx));

    switch (drawUI("Choose one option:", {"Sign In", "Sign Up"})) {
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
    project project(client);
    switch (drawUI("Choose one option:",
                   {"Create new project", "Open existing project",
                    "Download project from server"})) {
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
    }
    endwin();

    SSL_CTX_free(ctx);
    client.sock.close();
}
