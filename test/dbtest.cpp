#include <sqlite3.h>
#include <stdarg.h>

#include <filesystem>
#include <iostream>
#include <unordered_map>

using namespace std;
namespace fs = std::filesystem;

class clsDb {
   private:
    sqlite3 *db;
    unordered_map<string, unordered_map<string, string>> dbMap;

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
            while (sqlite3_step(stmt2) == SQLITE_ROW)
                dbMap[table][(const char *)sqlite3_column_text(stmt2, 1)] =
                    (const char *)sqlite3_column_text(stmt2, 2);

            sqlite3_finalize(stmt2);
        }
        sqlite3_finalize(stmt);
    }

   public:
    void insert(const string &table, const string &columns, ...) {
        va_list args;
        va_start(args, columns);
        short n = 0;
        for (int i = 0; i < columns.length(); i++)
            if (columns[i] == ',') n++;

        string command = "insert into users (" + columns + ") values (";
        for (int i = 0; i < n; i++) command += "?, ";
        command += "?);";
        cout << command << endl;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, command.c_str(), -1, &stmt, 0);

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
        "create table if not exists users(id integer primary key, username "
        "text, password blob); create table if not exists projects(id integer "
        "primary key, ownerId integer, prjName text, dir text, dirTree text)");
    db.insert("table", "username, password", "kurwa", "aboba");
}
