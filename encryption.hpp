#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

using std::vector, std::gcd, std::string;

vector<int> primeNums;

void genPrime(int n) {
    vector<bool> isPrime(n, true);
    isPrime[0] = false;
    isPrime[1] = false;
    for (int i = 2; i <= sqrt(n); i++)
        if (isPrime[i])
            for (int j = i * i; j < n; j += i) isPrime[j] = false;
    for (int i = 0; i < isPrime.size(); i++)
        if (isPrime[i]) primeNums.push_back(i);
}

void setKeys(int &n, int &publicKey, int &privateKey) {
    srand(static_cast<uint>(time(0)));
    int k = rand() % primeNums.size();
    vector<int>::iterator it = primeNums.begin();
    while (k--) it++;
    int p = *it;
    primeNums.erase(it);
    int q = primeNums[rand() % primeNums.size()];
    // cout << "p: " << p << " q: " << q << endl;

    n = p * q;
    // cout << "n: " << n << endl;
    int fi = (p - 1) * (q - 1);
    // cout << "fi: " << fi << endl;

    publicKey = 2;
    while (gcd(fi, publicKey) != 1) publicKey++;
    // cout << "publicKey: " << publicKey << endl;
    privateKey = 2;
    while (privateKey * publicKey % fi != 1) privateKey++;
    // cout << "privateKey: " << privateKey << endl;
}

unsigned int powMod(int n, int e, unsigned int ch) {
    unsigned res = 1;
    while (e--) res = res * ch % n;
    return res;
}
vector<unsigned int> encrypt(int n, int publicKey, std::string msg) {
    vector<unsigned int> res;
    for (char ch : msg) res.push_back(powMod(n, publicKey, ch));
    return res;
}
std::string decrypt(int n, int privateKey, vector<unsigned int> msg) {
    std::string res;
    for (unsigned int ch : msg) res += powMod(n, privateKey, ch);
    return res;
}
