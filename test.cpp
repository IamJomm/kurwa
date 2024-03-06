#include <chrono>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

using nlohmann::json, std::string;
using std::cout, std::endl;
namespace fs = std::filesystem;
namespace ch = std::chrono;

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

void newValues(json &js, string path) {
    for (json::iterator it = js.begin(); it != js.end(); ++it) {
        if (it.value().is_object()) {
            cout << path << it.key() << " was created" << endl;
            newValues(it.value(), path + it.key() + '/');
        } else
            cout << path << it.key() << " was created" << endl;
    }
}
bool compJson(json &first, json &second, string path) {
    bool res;
    for (json::iterator it = first.begin(); it != first.end(); ++it) {
        if (second.contains(it.key())) {
            if (it.value().is_object() && second[it.key()].is_object() ||
                it.value().is_null() && second[it.key()].is_object())
                compJson(it.value(), second[it.key()], path + it.key() + '/');
            else if (it.value() != second[it.key()])
                cout << path << it.key() << " was changed" << endl, res = false;
        } else
            cout << path << it.key() << " was deleted" << endl, res = false;
    }
    for (json::iterator it = second.begin(); it != second.end(); ++it)
        if (!first.contains(it.key())) {
            cout << path << it.key() << " was created" << endl;
            if (it.value().is_object())
                newValues(it.value(), path + it.key() + '/');
            res = false;
        }
    return res;
}

int main() {
    string path = "/home/jomm/Documents/kurwa/test/";
    json curr = genJson(path);
    while (true) {
        std::this_thread::sleep_for(ch::seconds(1));
        json check = genJson(path);
        // cout << check.dump(2) << endl;
        if (!compJson(curr, check, "./")) curr = check;
    }
}
