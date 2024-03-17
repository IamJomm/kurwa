#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

string prjPath = "/home/jomm/Documents/kurwa/client/test/";

void download(const string &path = prjPath) {
    for (const fs::directory_entry &entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            cout << "createDir "
                 << entry.path().string().substr(prjPath.length()) << endl;
            download(entry.path().string());
        } else {
            cout << "createFile "
                 << entry.path().string().substr(prjPath.length()) << endl;
        }
    }
}

int main() { download(); }
