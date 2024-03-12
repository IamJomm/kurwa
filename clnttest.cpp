#include <chrono>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

using namespace std;
using nlohmann::json;

namespace ch = std::chrono;
namespace fs = std::filesystem;

json genJson(string path) {
    json res;
    for (const fs::directory_entry &entry : fs::directory_iterator(path)) {
        if (entry.is_directory())
            res[entry.path().filename().string()] =
                genJson(entry.path().string());
        else
            res[entry.path().filename().string()] =
                ch::duration_cast<ch::seconds>(
                    fs::last_write_time(entry).time_since_epoch())
                    .count();
    }
    return res;
}

void compJson(json &oldjs, json &newjs, string path) {
    for (json::iterator it = oldjs.begin(); it != oldjs.end(); ++it) {
        if (newjs.contains(it.key())) {
            if (it.value().is_number() && newjs[it.key()].is_object()) {
                cout << "file " << path + it.key() << " was deleted" << endl
                     << "folder " << path + it.key() << " was created" << endl;
                json temp;
                compJson(temp, newjs[it.key()], path + it.key() + '/');
            } else if (it.value().is_object() && newjs[it.key()].is_number()) {
                cout << "folder " << path + it.key() << " was deleted" << endl
                     << "file " << path + it.key() << " was created" << endl;
            } else if (newjs[it.key()].is_object()) {
                compJson(it.value(), newjs[it.key()], path + it.key() + '/');
            } else if (it.value() != newjs[it.key()]) {
                cout << "file " << path + it.key() << " was changed" << endl;
            }
        } else {
            if (it.value().is_object() || it.value().is_null()) {
                cout << "folder " << path + it.key() << " was deleted" << endl;
            } else {
                cout << "file " << path + it.key() << " was deleted" << endl;
            }
        }
    }
    for (json::iterator it = newjs.begin(); it != newjs.end(); ++it) {
        if (!oldjs.contains(it.key())) {
            if (it.value().is_object() || it.value().is_null()) {
                cout << "folder " << path + it.key() << " was created" << endl;
                json temp;
                compJson(temp, it.value(), path + it.key() + '/');
            } else {
                cout << "file " << path + it.key() << " was created" << endl;
            }
        }
    }
}

int main() {
    json curr = genJson("/home/jomm/Documents/kurwa/test");
    while (true) {
        json check = genJson("/home/jomm/Documents/kurwa/test");
        compJson(curr, check, "./");
        curr = check;
        this_thread::sleep_for(ch::seconds(1));
    }
}
