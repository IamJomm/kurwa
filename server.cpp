#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "sr.hpp"

using std::string, std::thread, std::ref, std::cout, std::endl;
namespace fs = std::filesystem;

class db {
   private:
    sqlite3* database;

   public:
    bool exec(const string& query, const string& types, ...) {
        va_list args;
        va_start(args, types);
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(database, query.c_str(), -1, &stmt, 0);

        short toBind = 0;
        for (char ch : query)
            if (ch == '?') ++toBind;
        for (short i = 1, j = toBind; j; i++, j--) {
            switch (types[types.length() - j]) {
                case 's':
                    sqlite3_bind_text(stmt, i, va_arg(args, const char*), -1,
                                      SQLITE_STATIC);
                    break;
                case 'i':
                    sqlite3_bind_int(stmt, i, va_arg(args, unsigned long));
                    break;
                case 'b':
                    const unsigned char* blob =
                        va_arg(args, const unsigned char*);
                    size_t blobSize = va_arg(args, size_t);
                    sqlite3_bind_blob(stmt, i, blob, blobSize - 1,
                                      SQLITE_TRANSIENT);
                    break;
            }
        }
        bool found = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            for (short i = 0; i < types.length() - toBind; i++) {
                switch (types[i]) {
                    case 's': {
                        string* ptr = va_arg(args, string*);
                        *ptr = (const char*)sqlite3_column_text(stmt, i);
                        break;
                    }
                    case 'i': {
                        unsigned long* ptr = va_arg(args, unsigned long*);
                        *ptr = sqlite3_column_int(stmt, i);
                        break;
                    }
                    case 'b': {
                        unsigned char* blob = va_arg(args, unsigned char*);
                        size_t blobSize = va_arg(args, size_t);
                        memcpy(blob, sqlite3_column_blob(stmt, i),
                               blobSize - 1);
                        blob[blobSize - 1] = '\0';
                        break;
                    }
                }
            }
            found = true;
        }
        sqlite3_finalize(stmt);
        va_end(args);
        return found;
    }

    db(const string& path, const string& command) {
        sqlite3_open(path.c_str(), &database);
        sqlite3_exec(database, command.c_str(), 0, 0, 0);
    }
    ~db() { sqlite3_close(database); }
};

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

    void reg(db& db) {
        string sBuffer;
        string username;
        while (username.empty()) {
            sBuffer = sock.recv();
            if (!db.exec("select id from users where username = ?;", "s",
                         sBuffer.c_str())) {
                username = sBuffer;
                sock.send("ok");
            } else
                sock.send("not ok");
        }
        sBuffer = sock.recv();
        unsigned char hash[SHA256_DIGEST_LENGTH + 1];
        genSha256Hash(sBuffer, hash);
        db.exec("insert into users (username, password) values (?, ?);", "sb",
                username.c_str(), hash, sizeof(hash));
        cout << "[+] New user created." << endl;
    }

    void log(db& db) {
        while (!id) {
            string username = sock.recv();
            string password = sock.recv();
            unsigned char hash[SHA256_DIGEST_LENGTH + 1];
            genSha256Hash(password, hash);
            if (db.exec(
                    "select id from users where username = ? and password = ?;",
                    "isb", username.c_str(), hash, sizeof(hash), &id))
                sock.send("ok");
            else
                sock.send("not ok");
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

    void create(db& db) {
        while (!prjId) {
            string prjName = owner.sock.recv();
            if (!db.exec("select id from projects where ownerId = ? and "
                         "prjName = ?;",
                         "is", owner.id, prjName.c_str())) {
                uuid_t uuid;
                uuid_generate(uuid);
                char uuidStr[37];
                uuid_unparse(uuid, uuidStr);
                prjPath = prjPath + uuidStr + '/';
                cout << prjPath << endl;
                fs::create_directory(prjPath);
                db.exec(
                    "insert into projects (ownerId, prjName, dir, dirTree) "
                    "values (?, ?, ?, '{}');",
                    "iss", owner.id, prjName.c_str(), uuidStr, &prjId);
                db.exec(
                    "select id from projects where ownerId = ? and prjName = ?",
                    "iis", owner.id, prjName.c_str(), &prjId);
                cout << "[+] New project created." << endl;
                owner.sock.send("ok");
            } else
                owner.sock.send("not ok");
        }
    }

    void set(db& db) {
        while (!prjId) {
            string uuid;
            if (db.exec("select id, dir from projects where ownerId = ? "
                        "and prjName = ?;",
                        "isis", owner.id, owner.sock.recv().c_str(), &prjId,
                        &uuid)) {
                prjPath = prjPath + uuid + '/';
                cout << prjPath << endl;
                owner.sock.send("ok");
            } else
                owner.sock.send("not ok");
        }
    }

    void open(db& db) {
        string command;
        while ((command = owner.sock.recv()) != "quit") {
            if (command == "push") {
                string dirTree;
                db.exec("select dirTree from projects where id = ?;", "si",
                        prjId, &dirTree);
                cout << dirTree << endl;
                owner.sock.send(dirTree);
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
                db.exec("update projects set dirTree = ? where id = ?;", "si",
                        command.c_str(), prjId);
            }
        }
    }

    void download() {
        downloadHelp(prjPath);
        owner.sock.send("done");
    }

    project(client& x, const string& y) : owner(x), prjPath(y) {}
};

void handleClient(client client, db& db, const string& path) {
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
    const string path = fs::canonical(argv[0]).parent_path().string() + '/';

    db db(
        path + "users.db",
        "create table if not exists users (id integer primary key, username "
        "text, password blob); create table if not exists projects (id integer "
        "primary key, ownerId integer, prjName text, dir text, dirTree text)");

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
               ref(db), path)
            .detach();

    SSL_CTX_free(ctx);
    close(servSock);
}
