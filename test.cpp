#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

int main() { fs::remove_all("/home/jomm/Documents/kurwa/test/folder1"); }
