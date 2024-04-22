#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdarg.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <unordered_map>

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
    unordered_map<string, unordered_map<string, short>> dbMap;

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
                short iType;
                if (sType == "TEXT")
                    iType = 1;
                else if (sType == "INTEGER")
                    iType = 2;
                else if (sType == "BLOB")
                    iType = 3;

                dbMap[table][(const char *)sqlite3_column_text(stmt2, 1)] =
                    iType;
            }
            sqlite3_finalize(stmt2);
        }
        sqlite3_finalize(stmt);
    }

    void countChar(const string &str, char ch, short &n) {
        n = 0;
        for (short i = 0; i < str.length(); i++)
            if (str[i] == ',') n++;
    }

   public:
    void insert(const string &table, const string &columns, ...) {
        va_list args;
        va_start(args, columns);
        short n;
        countChar(columns, ',', n);

        string command = "insert into users (" + columns + ") values (";
        for (int i = 0; i < n; i++) command += "?, ";
        command += "?);";
        cout << command << endl;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, command.c_str(), -1, &stmt, 0);

        short left = 0, right;
        short column = 1;
        do {
            string columnName;
            right = columns.find(',', left);
            if (right != string::npos)
                columnName = columns.substr(left, right - left);
            else
                columnName = columns.substr(left);
            switch (dbMap[table][columnName]) {
                case 1:
                    sqlite3_bind_text(stmt, column, va_arg(args, const char *),
                                      -1, SQLITE_STATIC);
                    break;
                case 2:
                    sqlite3_bind_int(stmt, column, va_arg(args, unsigned long));
                    break;
                case 3:
                    int size = va_arg(args, int);
                    sqlite3_bind_blob(stmt, 2, va_arg(args, char *), size,
                                      SQLITE_TRANSIENT);
                    break;
            }
            left = right + 2;
            column++;
        } while (right != string::npos);
        sqlite3_step(stmt);

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
    db.insert("users", "username, password", "kurwa", sizeof(hash), hash);
}
