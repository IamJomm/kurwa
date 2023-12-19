#include <sys/socket.h>

#include <iostream>

using std::string;

void send(int socket, const string& msg) {
    const char* buffer = msg.c_str();
    send(socket, buffer, msg.size(), 0);
}

string recv(int socket) {
    string res;
    char buffer[1024];
    ssize_t bytes;
    while ((bytes = recv(socket, &buffer, sizeof(buffer), 0)) > 0)
        res.append(buffer, bytes);
    return res;
}
