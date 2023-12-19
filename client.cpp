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

int clientSock;

string drawUI(vector<string> arr) {
    noecho();
    int choice = 1, ch;
    bool kurwa = true;
    while (kurwa) {
        printw("%s\n", arr[0].c_str());
        for (int i = 1; i < arr.size(); i++)
            printw("%c %s\n", i == choice ? '>' : ' ', arr[i].c_str());
        switch (ch = getch()) {
            case 'k':
            case KEY_UP:
                if (choice > 1) choice--;
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

int main() {
    initscr();
    raw();
    keypad(stdscr, TRUE);

    clientSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr.s_addr);
    servAddr.sin_port = htons(8080);
    connect(clientSock, (sockaddr*)&servAddr, sizeof(servAddr));

    char buffer[1024];
    string sBuffer;
    if (drawUI({"Kurwa:", "Sign Up", "Sign In"}) == "Sign Up") {
        sBuffer = "Sign Up";
        send(clientSock, sBuffer);
        do {
            printw("Enter Username: ");
            getstr(buffer);
            send(clientSock, buffer);
        } while (recv(clientSock) != "ok");
        printw("Enter Password: ");
        getstr(buffer);
        send(clientSock, buffer);
        clear();
    } else {
    }

    endwin();
    close(clientSock);
}
