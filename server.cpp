#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <signal.h>
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

#include "sr.hpp"

using std::string, std::thread, std::cout, std::endl;
namespace fs = std::filesystem;

class client {
   private:
    void genSha256Hash(const string& input,
                       unsigned char hash[SHA256_DIGEST_LENGTH + 1]) {
        unsigned char buffer[input.length()];
        strcpy((char*)buffer, input.c_str());
        SHA256(buffer, strlen((char*)buffer), hash);
        hash[SHA256_DIGEST_LENGTH] = '\0';
    }

   public:
    clsSock sock;
    unsigned long id = 0;

    void reg(sqlite3* db) {
        string sBuffer;
        sqlite3_stmt* stmt;
        string username;
        while (username.empty()) {
            sBuffer = sock.recv();
            sqlite3_prepare_v2(db, "select id from users where username = ?;",
                               -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, sBuffer.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW)
                sock.send("not ok");
            else {
                username = sBuffer;
                sock.send("ok");
            }
            sqlite3_finalize(stmt);
        }
        sBuffer = sock.recv();
        unsigned char hash[SHA256_DIGEST_LENGTH + 1];
        genSha256Hash(sBuffer, hash);
        sqlite3_prepare_v2(
            db, "insert into users (username, password) values (?, ?);", -1,
            &stmt, 0);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 2, hash, SHA256_DIGEST_LENGTH,
                          SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        cout << "[+] New user created." << endl;
    }

    void log(sqlite3* db) {
        sqlite3_stmt* stmt;
        while (!id) {
            string username = sock.recv();
            string password = sock.recv();
            sqlite3_prepare_v2(
                db, "select id from users where username = ? and password = ?;",
                -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
            unsigned char hash[SHA256_DIGEST_LENGTH + 1];
            genSha256Hash(password, hash);
            sqlite3_bind_blob(stmt, 2, hash, SHA256_DIGEST_LENGTH,
                              SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                id = sqlite3_column_int(stmt, 0);
                sock.send("ok");
            } else
                sock.send("not ok");
            sqlite3_finalize(stmt);
        }
    }

    client(int sock, SSL* ssl) : sock(sock, ssl) {}
};

class project {
   private:
    void downloadHelp(const string& path) {
        for (const fs::directory_entry& entry : fs::directory_iterator(path)) {
            string filePath = entry.path().string();
            if (entry.is_directory()) {
                owner.sock.send("createDir " +
                                filePath.substr(prjPath.length()));
                downloadHelp(filePath);
            } else {
                owner.sock.send("createFile " +
                                filePath.substr(prjPath.length()));
                owner.sock.sendFile(filePath);
            }
        }
    }

   public:
    client owner;
    string prjPath;
    unsigned long prjId = 0;

    void create(sqlite3* db) {
        sqlite3_stmt* stmt;
        while (!prjId) {
            string prjName = owner.sock.recv();
            sqlite3_prepare_v2(
                db,
                "select id from projects where ownerId = ? and prjName = ?;",
                -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, owner.id);
            sqlite3_bind_text(stmt, 2, prjName.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) != SQLITE_ROW) {
                uuid_t uuid;
                uuid_generate(uuid);
                char uuidStr[37];
                uuid_unparse(uuid, uuidStr);
                prjPath = prjPath + uuidStr + '/';
                fs::create_directory(prjPath);
                sqlite3_finalize(stmt);
                sqlite3_prepare_v2(db,
                                   "insert into projects (ownerId, prjName, "
                                   "dir, dirTree) values (?, ?, ?, '{}');",
                                   -1, &stmt, 0);
                sqlite3_bind_int(stmt, 1, owner.id);
                sqlite3_bind_text(stmt, 2, prjName.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, uuidStr, -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                prjId = sqlite3_last_insert_rowid(db);
                cout << "[+] New project created." << endl;
                owner.sock.send("ok");
            } else
                owner.sock.send("not ok");
            sqlite3_finalize(stmt);
        }
    }

    void set(sqlite3* db) {
        sqlite3_stmt* stmt;

        while (!prjId) {
            sqlite3_prepare_v2(db,
                               "select id, dir from projects where ownerId = ? "
                               "and prjName = ?;",
                               -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, owner.id);
            sqlite3_bind_text(stmt, 2, owner.sock.recv().c_str(), -1,
                              SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                prjId = sqlite3_column_int(stmt, 0);
                prjPath =
                    prjPath + (const char*)sqlite3_column_text(stmt, 1) + '/';
                owner.sock.send("ok");
            } else
                owner.sock.send("not ok");
            sqlite3_finalize(stmt);
        }
    }

    void open(sqlite3* db) {
        sqlite3_stmt* stmt;

        string command;
        while ((command = owner.sock.recv()) != "quit") {
            if (command == "push") {
                sqlite3_prepare_v2(db,
                                   "select dirTree from projects where id = ?;",
                                   -1, &stmt, 0);
                sqlite3_bind_int(stmt, 1, prjId);
                sqlite3_step(stmt);
                owner.sock.send((const char*)sqlite3_column_text(stmt, 0));
                sqlite3_finalize(stmt);
                while ((command = owner.sock.recv())[0] != '{') {
                    string action = command.substr(0, command.find(' '));
                    string path =
                        prjPath + command.substr(command.find(' ') + 1);
                    if (path.find("../") != string::npos)
                        owner.sock.send("not ok");
                    else {
                        owner.sock.send("ok");
                        if (action == "createDir")
                            fs::create_directory(path);
                        else if (action == "removeDir")
                            fs::remove_all(path);
                        else if (action == "createFile")
                            owner.sock.recvFile(path);
                        else if (action == "removeFile")
                            fs::remove(path);
                    }
                }
                sqlite3_prepare_v2(db,
                                   "update projects set dirTree=? where id=?;",
                                   -1, &stmt, 0);
                sqlite3_bind_text(stmt, 1, command.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, prjId);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
    }

    void download() {
        downloadHelp(prjPath);
        owner.sock.send("done");
    }

    project(client& x, const string& y) : owner(x), prjPath(y) {}
};

void handleClient(client client, sqlite3* db, string path) {
    cout << "[+] Kurwa client connected." << endl;
    string command = client.sock.recv();
    if (command == "signUp") {
        client.reg(db);
        client.log(db);
    } else if (command == "signIn")
        client.log(db);
    project project(client, path);
    command = client.sock.recv();
    if (command == "createPrj") {
        project.create(db);
        project.open(db);
    } else if (command == "openPrj") {
        project.set(db);
        project.open(db);
    } else if (command == "downloadPrj") {
        project.set(db);
        project.download();
        project.open(db);
    }
    client.sock.close();
    cout << "[-] Client disconnected." << endl;
}

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    string path = fs::canonical(argv[0]).parent_path().string() + '/';

    sqlite3* db;
    if (sqlite3_open((path + "users.db").c_str(), &db) != SQLITE_OK) {
        perror("[!] open");
        return -1;
    }
    if (sqlite3_exec(db,
                     "create table if not exists users (id integer primary "
                     "key, username text, password blob); create table if not "
                     "exists projects (id integer primary key, ownerId "
                     "integer, prjName text, dir text, dirTree text)",
                     0, 0, 0) != SQLITE_OK) {
        perror("[!] exec");
        return -1;
    }

    SSL_library_init();
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());

    SSL_CTX_use_certificate_file(ctx, (path + "cert.pem").c_str(),
                                 SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, (path + "key.pem").c_str(),
                                SSL_FILETYPE_PEM);

    int servSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8080);
    servAddr.sin_addr.s_addr = INADDR_ANY;
    socklen_t addrLen = sizeof(servAddr);
    bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr));

    listen(servSock, 5);
    cout << "[!] Everything is ok :)" << endl;
    while (true)
        thread(handleClient,
               client(accept(servSock, (sockaddr*)&servAddr, &addrLen),
                      SSL_new(ctx)),
               db, path)
            .detach();

    SSL_CTX_free(ctx);
    close(servSock);

    sqlite3_close(db);
}
