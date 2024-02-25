#include <openssl/rsa.h>
#include <openssl/sha.h>

#include <cstring>
#include <iomanip>
#include <iostream>

using std::string, std::cout, std::endl;

int main() {
    string sBuffer = "hui";
    unsigned char buffer[sBuffer.length()];
    strcpy((char*)buffer, sBuffer.c_str());
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(buffer, strlen((char*)buffer), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    cout << ss.str() << endl;
}
