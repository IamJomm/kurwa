#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdarg.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

void genSha256Hash(const string &input, char res[SHA256_DIGEST_LENGTH]) {
    unsigned char buffer[input.length()];
    strcpy((char *)buffer, input.c_str());
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(buffer, strlen((char *)buffer), hash);
    memcpy(res, hash, SHA256_DIGEST_LENGTH);
}

class clsDb {
   private:
    sqlite3 *db;
    enum columnType { TEXT = 1, INTEGER, BLOB };
    unordered_map<string, unordered_map<string, columnType>> dbMap;

    void setDbMap() {
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db,
                           "select name from sqlite_master where type='table'",
                           -1, &stmt, 0);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            string table = (const char *)sqlite3_column_text(stmt, 0);
            sqlite3_stmt *stmt2;
            sqlite3_prepare_v2(db, ("pragma table_info(" + table + ")").c_str(),
                               -1, &stmt2, 0);
            while (sqlite3_step(stmt2) == SQLITE_ROW) {
                string sType = (const char *)sqlite3_column_text(stmt2, 2);
                columnType iType;
                if (sType == "TEXT")
                    iType = TEXT;
                else if (sType == "INTEGER")
                    iType = INTEGER;
                else if (sType == "BLOB")
                    iType = BLOB;

                dbMap[table][(const char *)sqlite3_column_text(stmt2, 1)] =
                    iType;
            }
            sqlite3_finalize(stmt2);
        }
        sqlite3_finalize(stmt);
    }

   public:
    void insert(const string &table, const vector<string> &columns, ...) {
        va_list args;
        va_start(args, columns);

        string command = "insert into users (" + columns[0];
        int n = columns.size();
        for (int i = 1; i < n; i++) command += ',' + columns[i];
        command += ") values (";
        for (int i = 0; i < n - 1; i++) command += "?,";
        command += "?);";

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, command.c_str(), -1, &stmt, 0);

        for (int i = 0; i < n; i++) {
            switch (dbMap[table][columns[i]]) {
                case TEXT:
                    sqlite3_bind_text(stmt, i + 1, va_arg(args, const char *),
                                      -1, SQLITE_STATIC);
                    break;
                case INTEGER:
                    sqlite3_bind_int(stmt, i + 1, va_arg(args, unsigned long));
                    break;
                case BLOB:
                    int size = va_arg(args, int);
                    sqlite3_bind_blob(stmt, i + 1, va_arg(args, const char *),
                                      size, SQLITE_TRANSIENT);
                    break;
            }
        }

        sqlite3_step(stmt);

        sqlite3_finalize(stmt);
        va_end(args);
    }
    void select(const string &table, const vector<string> &columns,
                const string &condition, ...) {
        va_list args;
        va_start(args, condition);

        string command = "select " + columns[0];
        int n = columns.size();
        for (int i = 1; i < n; i++) command += ',' + columns[i];
        command += " from " + table;

        if (!condition.empty())
            command += " where " + condition + ';';
        else
            command += ';';
        cout << command << endl;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, command.c_str(), -1, &stmt, 0);

        if (!condition.empty()) {
            short left = 0, right;
            short column = 1;
            while ((right = condition.find('=', left)) != string::npos) {
                if ((left = condition.rfind(' ', right - 2)) == string::npos)
                    left = 0;
                else
                    left++;
                switch (
                    dbMap[table][condition.substr(left, right - left - 1)]) {
                    case TEXT:
                        sqlite3_bind_text(stmt, column,
                                          va_arg(args, const char *), -1,
                                          SQLITE_STATIC);
                        break;
                    case INTEGER:
                        sqlite3_bind_int(stmt, column,
                                         va_arg(args, unsigned long));
                        break;
                    case BLOB:
                        int size = va_arg(args, int);
                        sqlite3_bind_blob(stmt, column,
                                          va_arg(args, const char *), size,
                                          SQLITE_TRANSIENT);
                        break;
                }
                left = right + 1;
                column++;
            }
        }
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            for (int i = 0; i < n; i++) {
                switch (dbMap[table][columns[i]]) {
                    case TEXT: {
                        string *ptr = va_arg(args, string *);
                        *ptr = (const char *)sqlite3_column_text(stmt, i);
                        break;
                    }
                    case INTEGER: {
                        unsigned long *ptr = va_arg(args, unsigned long *);
                        *ptr = sqlite3_column_int(stmt, i);
                        break;
                    }
                    case BLOB: {
                        int size = va_arg(args, int);
                        char *ptr = va_arg(args, char *);
                        memcpy(ptr, sqlite3_column_blob(stmt, i), size);
                        break;
                    }
                }
            }
        }

        sqlite3_finalize(stmt);
        va_end(args);
    }

    clsDb(const string &path, const string &command) {
        sqlite3_open(path.c_str(), &db);
        sqlite3_exec(db, command.c_str(), 0, 0, 0);
        setDbMap();
    }
};

int main(int argc, char *argv[]) {
    string path = fs::canonical(argv[0]).parent_path().string() + '/';

    clsDb db(
        path + "test.db",  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        "create table if not exists users (id integer primary key, username "
        "text, password blob); create table if not exists projects (id integer "
        "primary key, ownerId integer, prjName text, dir text, dirTree text)");

    char hash[SHA256_DIGEST_LENGTH];
    genSha256Hash("kurwa", hash);
    // db.insert("users", {"username", "password"}, "bober", sizeof(hash),
    // hash);
    string username;
    char password[SHA256_DIGEST_LENGTH];
    db.select("users", {"username", "password"}, "id = ? ", 1, &username,
              sizeof(password), &password);
    cout << username << '|' << password << endl;
}
// insert into users (username, password) values (?, ?);
// update projects set dirTree = ? where id = ?;
// select id, dir from projects where ownerId = ? and prjName = ?;
