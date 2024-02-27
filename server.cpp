#include <netinet/in.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "encryption.hpp"
#include "sr.hpp"

using std::string, std::thread, std::cout, std::endl, std::stringstream;

const char* genSha256Hash(string& input) {
    unsigned char buffer[input.length()];
    strcpy((char*)buffer, input.c_str());
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(buffer, strlen((char*)buffer), hash);
    char* res = new char[SHA256_DIGEST_LENGTH];
    memcpy(res, hash, SHA256_DIGEST_LENGTH);
    return res;
}

class client {
   public:
    clsSock sock;
    unsigned long id = 0;

    void reg(sqlite3* db) {
        string sBuffer;
        string sql;
        sqlite3_stmt* stmt;
        string username;
        while (true) {
            sBuffer = sock.recv();
            sql = "SELECT username FROM users WHERE username = ?;";
            sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, sBuffer.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW)
                sock.send("not ok");
            else {
                username = sBuffer;
                sock.send("ok");
                break;
            }
            sqlite3_finalize(stmt);
        }
        sBuffer = sock.recv();
        const char* password = genSha256Hash(sBuffer);
        sql = "INSERT INTO users (username, password) VALUES (?, ?);";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 2, password, SHA256_DIGEST_LENGTH,
                          SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        delete[] password;
        sqlite3_finalize(stmt);
        cout << "[+] New user created." << endl;
    }

    void log(sqlite3* db) {
        string sql;
        sqlite3_stmt* stmt;
        do {
            string username = sock.recv();
            string password = sock.recv();
            sql = "SELECT id, password FROM users WHERE username = ?;";
            sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* dbHash = (const char*)sqlite3_column_text(stmt, 1);
                const char* hash = genSha256Hash(password);
                if (!strcmp(dbHash, hash)) {
                    id = sqlite3_column_int(stmt, 0);
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
        "TEXT, password BLOB);";
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
