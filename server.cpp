#include <netinet/in.h>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "encryption.hpp"
#include "sr.hpp"

using std::string, std::thread, std::cout, std::endl;
namespace fs = std::filesystem;

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
            sql = "select id from users where username = ?;";
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
        sql = "insert into users (username, password) values (?, ?);";
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
            sql = "select id, password from users where username = ?;";
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
                delete[] hash;
            } else
                sock.send("not ok");
            sqlite3_finalize(stmt);
        } while (!id);
    }

    client(int x) : sock(x) {}
};

class project {
   public:
    client owner;
    string prjPath;
    string prjName;

    void create(sqlite3* db, string& path) {
        string sql;
        sqlite3_stmt* stmt;
        do {
            prjName = owner.sock.recv();
            cout << prjName << endl;
            sql = "select id from projects where ownerId = ? and prjName = ?;";
            sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, owner.id);
            sqlite3_bind_text(stmt, 2, prjName.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) != SQLITE_ROW) {
                uuid_t uuid;
                uuid_generate(uuid);
                char uuidStr[37];
                uuid_unparse(uuid, uuidStr);
                fs::create_directory(path + uuidStr);
                sqlite3_finalize(stmt);
                sql =
                    "insert into projects (ownerId, prjName, dir) values (?, "
                    "?,?);";
                sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
                sqlite3_bind_int(stmt, 1, owner.id);
                sqlite3_bind_text(stmt, 2, prjName.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, uuidStr, -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                prjPath = uuidStr;
                owner.sock.send("ok");
            } else
                owner.sock.send("not ok");
            sqlite3_finalize(stmt);
        } while (prjPath.empty());
    }
    void open(sqlite3* db) {}
    void download(sqlite3* db) {}

    project(client& x) : owner(x) {}
};

void handleClient(client client, sqlite3* db, string path) {
    cout << "[+] New kurwa client connected." << endl;
    string command = client.sock.recv();
    if (command == "sign up") {
        client.reg(db);
        client.log(db);
    } else
        client.log(db);
    project project(client);
    command = client.sock.recv();
    if (command == "create prj") {
        project.create(db, path);
        project.open(db);
    } else if (command == "open prj")
        project.open(db);
    else {
        project.download(db);
        project.open(db);
    }
    close(client.sock.sock);
}

int main(int argc, char* argv[]) {
    string path(argv[0]);
    path = path.substr(0, path.rfind('/') + 1);

    sqlite3* db;
    if (sqlite3_open((path + "users.db").c_str(), &db) != SQLITE_OK) {
        perror("[!] open");
        return -1;
    }
    string sql =
        "create table if not exists users(id integer primary key, username "
        "text, password blob); create table if not exists projects(id integer "
        "primary key, ownerId integer, prjName text, dir text)";
    if (sqlite3_exec(db, sql.c_str(), 0, 0, 0) != SQLITE_OK) {
        perror("[!] exec");
        return -1;
    }

    int servSock = socket(AF_INET, SOCK_STREAM, 0), opt = 1;
    if (servSock == -1) {
        perror("[!] socket");
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
        perror("[!] bind");
        return -1;
    }
    listen(servSock, 5);
    cout << "[!] Everything is ok :)" << endl;
    while (true)
        thread(handleClient,
               client(accept(servSock, (sockaddr*)&servAddr, &addrLen)), db,
               path)
            .detach();
    close(servSock);
    sqlite3_close(db);
}
