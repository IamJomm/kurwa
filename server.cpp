#include <netinet/in.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>

#include "encryption.hpp"
#include "sr.hpp"

using std::cout, std::endl;
using std::string, std::thread;

void clientReg(clsSock& clientSock, sqlite3* db) {
    string sBuffer;
    string sql;
    sqlite3_stmt* stmt;
    string username;

    while (true) {
        sBuffer = clientSock.recv();
        sql =
            "SELECT username FROM users WHERE username = \'" + sBuffer + "\';";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            clientSock.send("not ok");
        else {
            username = sBuffer;
            clientSock.send("ok");
            break;
        }
    }

    sBuffer = clientSock.recv();
    unsigned char buffer[sBuffer.length()];
    strcpy((char*)buffer, sBuffer.c_str());
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(buffer, strlen((char*)buffer), hash);
    string formatedHash(reinterpret_cast<const char*>(hash),
                        SHA256_DIGEST_LENGTH);
    sql = "INSERT INTO users (username, password) VALUES (\'" + username +
          "\', \'" + formatedHash + "\');";
    sqlite3_exec(db, sql.c_str(), 0, 0, 0);

    sqlite3_finalize(stmt);
}

long clientLog(clsSock& clientSock, sqlite3* db) {}

void handleClient(clsSock clientSock, sqlite3* db) {
    cout << "[+] New kurwa client connected." << endl;

    long clientId;
    if (clientSock.recv() == "Sign Up") {
        clientReg(clientSock, db);
        clientId = clientLog(clientSock, db);
    } else
        clientId = clientLog(clientSock, db);
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
    if (servSock == -1) {
        cout << "[!] Cannot create socket.";
        return -1;
    }
    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
               sizeof(opt));
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(8080);
    socklen_t addrLen = sizeof(servAddr);
    if (bind(servSock, (sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        cout << "[!] Cannot bind socket.";
        return -1;
    }
    if (listen(servSock, 5) == -1) {
        cout << "[!] Idi nahui :3";
        return -1;
    }
    cout << "[!] Everything is ok :)" << endl;
    while (true) {
        int clientSock = accept(servSock, (sockaddr*)&servAddr, &addrLen);
        if (clientSock == -1) {
            cout << "[!] Cannot connect client :)";
        }
        thread(handleClient, clsSock(clientSock), db).detach();
    }
    close(servSock);
    sqlite3_close(db);
}
