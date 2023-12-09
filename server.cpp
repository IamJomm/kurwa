#include <netinet/in.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <thread>

using std::string;

using std::cout, std::endl, std::thread;

void handeClient(int clientSock) {
    char buffer[1024];
    recv(clientSock, &buffer, sizeof(buffer), 0);
    char msg[1024];
    snprintf(msg, sizeof(msg), "Hello, %s!!!", buffer);
    send(clientSock, &msg, sizeof(msg), 0);
}

int main(int argc, char* argv[]) {
    string path(argv[0]);
    path = path.substr(0, path.rfind('/') + 1);
    path += "users.db";

    sqlite3* db;
    sqlite3_open(path.c_str(), &db);
    string sql =
        "CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY, nickname "
        "TEXT, password TEXT)";
    char* sqlError;
    sqlite3_exec(db, sql.c_str(), NULL, 0, &sqlError);

    int servSock = socket(AF_INET, SOCK_STREAM, 0), opt = 1;
    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
               sizeof(opt));
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(8080);
    socklen_t addrLen = sizeof(servAddr);
    bind(servSock, (sockaddr*)&servAddr, sizeof(servAddr));
    listen(servSock, 5);
    while (true)
        thread(handeClient, accept(servSock, (sockaddr*)&servAddr, &addrLen))
            .detach();
    close(servSock);
    sqlite3_close(db);
}
