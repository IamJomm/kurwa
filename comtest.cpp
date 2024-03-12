#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "sr.hpp"

using namespace std;

void progressBar(int x) {}

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
    printw(client.recv().c_str());

    client.recvFile("/home/jomm/Documents/kurwa/test/aaa1");
    client.recvFile("/home/jomm/Documents/kurwa/test/abc1");

    close(servSock);
}

void clientSock() {
    clsSock client;
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr.s_addr);
    servAddr.sin_port = htons(8080);
    connect(client.sock, (sockaddr *)&servAddr, sizeof(servAddr));
    client.send("ok");

    client.sendFile("/home/jomm/Documents/kurwa/test/aaa", progressBar);
    client.sendFile("/home/jomm/Documents/kurwa/test/abc", progressBar);

    close(client.sock);
}

int main() {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    thread serverThread(serverSock);
    this_thread::sleep_for(chrono::seconds(1));
    thread clientThread(clientSock);
    serverThread.join();
    clientThread.join();
    endwin();
}
