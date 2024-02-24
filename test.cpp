#include <openssl/rsa.h>
#include <openssl/sha.h>

#include <cstring>
#include <iostream>

using std::string, std::cout, std::endl;

int main() {
    string sBuffer = "hui";
    unsigned char buffer[sBuffer.length()];
    strcpy((char*)buffer, sBuffer.c_str());
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(buffer, strlen((char*)buffer), hash);
    cout << hash << endl;
}
