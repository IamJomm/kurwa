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

using std::string, std::thread, std::cout, std::endl;

class client {
   public:
    clsSock sock;
    string username;
    unsigned long id = 0;

    void reg(sqlite3* db) {
        string sBuffer;
        string sql;
        sqlite3_stmt* stmt;
        while (true) {
            sBuffer = sock.recv();
            sql = "SELECT username FROM users WHERE username = \'" + sBuffer +
                  "\';";
            sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
            if (sqlite3_step(stmt) == SQLITE_ROW)
                sock.send("not ok");
            else {
                username = sBuffer;
                sock.send("ok");
                break;
            }
        }
        sBuffer = sock.recv();
        unsigned char buffer[sBuffer.length()];
        strcpy((char*)buffer, sBuffer.c_str());
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(buffer, strlen((char*)buffer), hash);
        string formatedHash(reinterpret_cast<const char*>(hash),
                            SHA256_DIGEST_LENGTH);
        sql = "INSERT INTO users (username, password) VALUES (\'" + username +
              "\', \'" + formatedHash + "\');";
        cout << sql << endl;
        sqlite3_exec(db, sql.c_str(), 0, 0, 0);
        sqlite3_finalize(stmt);
    }

    void log(sqlite3* db) {
        string sql;
        sqlite3_stmt* stmt;
        do {
            string recvdUsername = sock.recv();
            string recvdPassword = sock.recv();
            sql = "SELECT id, password FROM users WHERE username = \'" +
                  recvdUsername + "\';";
            sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                unsigned char buffer[recvdPassword.length()];
                strcpy((char*)buffer, recvdPassword.c_str());
                unsigned char hash[SHA256_DIGEST_LENGTH];
                SHA256(buffer, strlen((char*)buffer), hash);
                string formatedHash(reinterpret_cast<const char*>(hash),
                                    SHA256_DIGEST_LENGTH);
                string dbHash(
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                    SHA256_DIGEST_LENGTH);
                if (formatedHash == dbHash) {
                    id = sqlite3_column_int(stmt, 0);
                    username = recvdUsername;
                    sock.send("ok");
                } else
                    sock.send("not ok");
            } else
                sock.send("not ok");
        } while (!id);
        sqlite3_finalize(stmt);
    }

    client(int x) : sock(x) {}
};

void handleClient(client client, sqlite3* db) {
    cout << "[+] New kurwa client connected." << endl;
    if (client.sock.recv() == "Sign Up") {
        client.reg(db);
        client.log(db);
    } else
        client.log(db);
    cout << client.id << endl;
    close(client.sock.sock);
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
    listen(servSock, 5);
    cout << "[!] Everything is ok :)" << endl;
    while (true)
        thread(handleClient,
               client(accept(servSock, (sockaddr*)&servAddr, &addrLen)), db)
            .detach();
    close(servSock);
    sqlite3_close(db);
}
