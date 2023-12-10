#include <iostream>
#include <vector>

#include "rsa.hpp"
using namespace std;
int main() {
    int n, publicKey, privateKey;
    genPrime(250);
    setKeys(n, publicKey, privateKey);
    vector<unsigned int> arr = encrypt(n, publicKey, "Hello, world!");
    cout << "Encrypted message: ";
    for (int x : arr) cout << x << " ";
    cout << endl << "Decrypted message: ";
    cout << decrypt(n, privateKey, arr);
}
