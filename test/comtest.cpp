#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <thread>

#include "../sr.hpp"

using namespace std;

void progressBar(long prog, long total) {
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

void serverSock() {
    int servSock = socket(AF_INET, SOCK_STREAM, 0), opt = 1;
    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
               sizeof(opt));
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(8080);
    socklen_t addrLen = sizeof(servAddr);
    bind(servSock, (sockaddr *)&servAddr, sizeof(servAddr));
    listen(servSock, 3);
    clsSock client(accept(servSock, (sockaddr *)&servAddr, &addrLen));

    client.recvFile("/home/jomm/Documents/kurwa/client/test/aaa1");
    client.recvFile("/home/jomm/Documents/kurwa/client/test/Asakusa1.png");
    client.recvFile("/home/jomm/Documents/kurwa/client/test/bbb1");

    close(servSock);
}

void clientSock() {
    clsSock client;
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr.s_addr);
    servAddr.sin_port = htons(8080);
    connect(client.sock, (sockaddr *)&servAddr, sizeof(servAddr));

    printw("aaa\n");
    client.sendFile("/home/jomm/Documents/kurwa/client/test/aaa", progressBar);
    printw("Asakusa.png\n");
    client.sendFile("/home/jomm/Documents/kurwa/client/test/Asakusa.png",
                    progressBar);
    printw("abc\n");
    client.sendFile("/home/jomm/Documents/kurwa/client/test/bbb", progressBar);

    close(client.sock);
}

int main() {
    setlocale(LC_ALL, "");
    initscr();
    scrollok(stdscr, TRUE);

    thread serverThread(serverSock);
    this_thread::sleep_for(chrono::seconds(1));
    thread clientThread(clientSock);
    serverThread.join();
    clientThread.join();

    getch();
    endwin();
}
