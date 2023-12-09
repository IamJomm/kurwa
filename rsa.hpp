#include <math.h>

#include <iostream>
#include <numeric>
#include <vector>

using std::cout, std::endl;
using std::vector, std::gcd;

vector<int> primeNums;
int n, publicKey, privateKey;

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

void setKeys() {
    srand(static_cast<uint>(time(0)));
    int k = rand() % primeNums.size();
    vector<int>::iterator it = primeNums.begin();
    while (k--) it++;
    int p = *it;
    primeNums.erase(it);
    int q = primeNums[rand() % primeNums.size()];
    cout << p << " " << q << endl;

    n = p * q;
    cout << n << endl;
    int fi = (p - 1) * (q - 1);
    cout << fi << endl;

    int publicKey = 2;
    while (gcd(fi, publicKey) != 1) publicKey++;
    cout << publicKey << endl;
    int privateKey = 2;
    while (privateKey * publicKey % fi != 1) privateKey++;
    cout << privateKey << endl;
}
