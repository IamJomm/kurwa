#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "sr.hpp"

using std::string, std::vector, std::cout, std::endl, std::cin;

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
   public:
    client owner;
    string prjName;
    string prjPath;

    void create() {
        char buffer[1024];
        printw("Path to your project: ");
        getstr(buffer);
        prjPath = buffer;
        do {
            printw("Name of your project: ");
            getstr(buffer);
            prjName = buffer;
        } while (owner.sock.recv() == "ok");
        owner.sock.send(prjName);
    }
    void open() {}
    void download() {}

    project(client& x) : owner(x) {}
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
    if (connect(client.sock.sock, (sockaddr*)&servAddr, sizeof(servAddr)) ==
        -1) {
        perror("[!] connect");
        return -1;
    }

    if (drawUI("Choose one option:", {"Sign Up", "Sign In"})[5] == 'U') {
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
            project.create();
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
