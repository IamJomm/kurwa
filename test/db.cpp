#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdarg.h>

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

void genSha256Hash(string input, unsigned char hash[SHA256_DIGEST_LENGTH + 1]) {
    unsigned char buffer[input.length()];
    strcpy((char *)buffer, input.c_str());
    SHA256(buffer, strlen((char *)buffer), hash);
    hash[SHA256_DIGEST_LENGTH] = '\0';
}

class clsDb {
   private:
    sqlite3 *db;

   public:
    bool exec(const string &query, const string &types, ...) {
        va_list args;
        va_start(args, types);
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, 0);
        short toBind = 0;
        for (char ch : query)
            if (ch == '?') ++toBind;
        for (short i = 1, j = toBind; j; i++, j--) {
            switch (types[types.length() - j]) {
                case 's':
                    sqlite3_bind_text(stmt, i, va_arg(args, const char *), -1,
                                      SQLITE_STATIC);
                    break;
                case 'i':
                    sqlite3_bind_int(stmt, i, va_arg(args, unsigned long));
                    break;
                case 'b':
                    const unsigned char *blob =
                        va_arg(args, const unsigned char *);
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
                        string *ptr = va_arg(args, string *);
                        *ptr = (const char *)sqlite3_column_text(stmt, i);
                        break;
                    }
                    case 'i': {
                        unsigned long *ptr = va_arg(args, unsigned long *);
                        *ptr = sqlite3_column_int(stmt, i);
                        break;
                    }
                    case 'b': {
                        unsigned char *blob = va_arg(args, unsigned char *);
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

    clsDb(const string &path, const string &command) {
        sqlite3_open(path.c_str(), &db);
        sqlite3_exec(db, command.c_str(), 0, 0, 0);
    }
    ~clsDb() { sqlite3_close(db); }
};

int main(int argc, char *argv[]) {
    const string path = fs::canonical(argv[0]).parent_path().string() + '/';

    clsDb db(
        path + "test.db",  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        "create table if not exists users (id integer primary key, username "
        "text, password blob); create table if not exists projects (id integer "
        "primary key, ownerId integer, prjName text, dir text, dirTree text)");

    unsigned char hash[SHA256_DIGEST_LENGTH + 1];
    genSha256Hash("kurwa", hash);

    unsigned long id = 0;
    string username;
    unsigned char password[SHA256_DIGEST_LENGTH + 1];
    cout << db.exec("insert into users (username, password) values (?, ?);",
                    "sb", "kurwa", hash, sizeof(hash))
         << ' ' << id << '|' << username << '|' << password << endl;
}
// insert into users (username, password) values (?, ?);
// update projects set dirTree = ? where id = ?;
// select id, dir from projects where ownerId = ? and prjName = ?;
