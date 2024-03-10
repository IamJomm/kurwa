#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

#include "sr.hpp"
using std::string, std::vector, std::cout, std::endl, std::cin, nlohmann::json;
namespace fs = std::filesystem;
namespace ch = std::chrono;

string drawUI(string title, vector<string> arr) {
    noecho();
    int choice = 0, ch;
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
    return arr[choice];
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

    void newValues(json &js, string path) {
        for (json::iterator it = js.begin(); it != js.end(); ++it) {
            if (it.value().is_object()) {
                cout << "folder " << path << it.key() << " was created" << endl;
                owner.sock.send("createDir " + path + it.key());
                newValues(it.value(), path + it.key() + '/');
            } else {
                cout << "file " << path << it.key() << " was created" << endl;
                owner.sock.send("createFile " + path + it.key());
                owner.sock.sendFile(prjPath + path + it.key());
            }
        }
    }
    void compJson(json &first, json &second, string path) {
        for (json::iterator it = first.begin(); it != first.end(); ++it) {
            if (second.contains(it.key())) {
                if (it.value().is_object() && second[it.key()].is_object() ||
                    it.value().is_null() && second[it.key()].is_object())
                    compJson(it.value(), second[it.key()],
                             path + it.key() + '/');
                else if (it.value() != second[it.key()]) {
                    cout << path << it.key() << " was changed" << endl;
                    owner.sock.send("createFile " + path + it.key());
                    owner.sock.sendFile(prjPath + path + it.key());
                }
            } else if (it.value().is_object() || it.value().is_null()) {
                cout << "folder " << path << it.key() << " was deleted" << endl;
                owner.sock.send("removeDir " + path + it.key());
            } else {
                cout << "file " << path << it.key() << " was deleted" << endl;
                owner.sock.send("removeFile " + path + it.key());
            }
        }
        for (json::iterator it = second.begin(); it != second.end(); ++it)
            if (!first.contains(it.key())) {
                if (it.value().is_object() || it.value().is_null()) {
                    cout << "folder " << path << it.key() << " was created"
                         << endl;
                    owner.sock.send("createDir " + path + it.key());
                    if (it.value().is_object())
                        newValues(it.value(), path + it.key() + '/');
                } else {
                    cout << "file " << path << it.key() << " was created"
                         << endl;
                    owner.sock.send("createFile " + path + it.key());
                    owner.sock.sendFile(prjPath + path + it.key());
                }
            }
    }

   public:
    client owner;
    string prjName;
    string prjPath;

    void set() {
        char buffer[1024];
        /*printw("Path to your project: ");
        getstr(buffer);
        prjPath = buffer;*/
        prjPath = "/home/jomm/Documents/kurwa/test/";
        do {
            printw("Name of your project: ");
            getstr(buffer);
            prjName = buffer;
            owner.sock.send(prjName);
        } while (owner.sock.recv() != "ok");
        prjName = buffer;
        clear();
    }
    void open() {
        if (owner.sock.recv() == "not ok") set();
        char buffer[20];
        while (true) {
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
    void download() {}

    project(client &x) : owner(x) {}
};

int main() {
    initscr();
    raw();
    keypad(stdscr, TRUE);

    client client;
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr.s_addr);
    servAddr.sin_port = htons(8080);
    if (connect(client.sock.sock, (sockaddr *)&servAddr, sizeof(servAddr)) ==
        -1) {
        perror("[!] connect");
        return -1;
    }

    if (drawUI("Choose one option:", {"Sign In", "Sign Up"})[5] == 'U') {
        client.sock.send("sign up");
        client.reg();
        client.log();
    } else {
        client.sock.send("sign in");
        client.log();
    }
    project project(client);
    switch (drawUI("Choose one option:",
                   {"Create new project", "Open existing project",
                    "Download project from server"})[0]) {
        case 'C':
            client.sock.send("create prj");
            project.set();
            project.open();
            break;
        case 'O':
            client.sock.send("open prj");
            project.open();
            break;
        case 'D':
            client.sock.send("download prj");
            project.download();
            project.open();
            break;
    }

    endwin();
    close(client.sock.sock);
}
