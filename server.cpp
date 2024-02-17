#include <netinet/in.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "encryption.hpp"
#include "sr.hpp"

using std::cout, std::endl;
using std::string, std::thread;

void handeClient(clsSock clientSock, sqlite3* db) {
    char buffer[1024];
    string sBuffer;
    string sql;
    sqlite3_stmt* stmt;
    cout << "abc" << endl;
    if (clientSock.recv() == "Sign Up") {
        sBuffer = clientSock.recv();
        cout << sBuffer << endl;
        sql =
            "SELECT username FROM users WHERE username = \"" + sBuffer + "\";";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        cout << "aaa" << endl;
        sqlite3_step(stmt);
        cout << stmt << endl << sqlite3_column_int(stmt, 0);
    }
}

int main(int argc, char* argv[]) {
    string path(argv[0]);
    path = path.substr(0, path.rfind('/') + 1);
    path += "users.db";

    sqlite3* db;
    sqlite3_open(path.c_str(), &db);
    string sql =
        "CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY, username "
        "TEXT, password TEXT);";
    sqlite3_exec(db, sql.c_str(), 0, 0, 0);

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
        thread(handeClient,
               clsSock(accept(servSock, (sockaddr*)&servAddr, &addrLen)), db)
            .detach();
    close(servSock);
    sqlite3_close(db);
}
