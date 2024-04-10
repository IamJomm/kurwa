// 64 bits is maximum you can get in c++ so it's implemented to do so
// you can edit constexpr var in getRandom64() to get lower amount of bits
#include <openssl/rsa.h>

#include <bitset>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

uint64_t mulmod(uint64_t a, uint64_t b, uint64_t m) {
    int64_t res = 0;

    while (a != 0) {
        if (a & 1) {
            res = (res + b) % m;
        }
        a >>= 1;
        b = (b << 1) % m;
    }
    return res;
}

uint64_t powMod(uint64_t a, uint64_t b, uint64_t n) {
    uint64_t x = 1;

    a %= n;

    while (b > 0) {
        if (b % 2 == 1) {
            x = mulmod(x, a, n);  // multiplying with base
        }
        a = mulmod(a, a, n);  // squaring the base
        b >>= 1;
    }
    return x % n;
}

std::vector<int> first_primes = {
    2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,
    47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103, 107,
    109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
    191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263,
    269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349};

// going through all 64 bits and placing randomly 0s and 1s
// setting first and last bit to 1 to get 64 odd number
uint64_t getRandom64() {
    // the value need to be 63 bits because you can not using 64 bit values do
    // a^2 which is needed
    constexpr int bits = 20;
    std::bitset<bits> a;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<short> distr(0, 1);

    for (int i = 0; i < bits; i++) {
        a[i] = distr(gen);
    }

    a[0] = 1;
    a[bits - 1] = 1;

    return a.to_ullong();
}

uint64_t getLowLevelPrime() {
    while (true) {
        uint64_t candidate = getRandom64();
        bool is_prime = true;
        for (int i = 0; i < first_primes.size(); i++) {
            if (candidate == first_primes[i]) return candidate;

            if (candidate % first_primes[i] == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime) return candidate;
    }
}

bool trialComposite(uint64_t a, uint64_t evenC, uint64_t to_test,
                    int max_div_2) {
    if (powMod(a, evenC, to_test) == 1) return false;

    for (int i = 0; i < max_div_2; i++) {
        uint64_t temp = static_cast<uint64_t>(1) << i;
        if (powMod(a, temp * evenC, to_test) == to_test - 1) return false;
    }

    return true;
}

bool MillerRabinTest(uint64_t to_test) {
    constexpr int accuracy = 20;

    int max_div_2 = 0;
    uint64_t evenC = to_test - 1;
    while (evenC % 2 == 0) {
        evenC >>= 1;
        max_div_2++;
    }

    // random numbers init
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> distr(2, to_test);

    for (int i = 0; i < accuracy; i++) {
        uint64_t a = distr(gen);

        if (trialComposite(a, evenC, to_test, max_div_2)) {
            return false;
        }
    }

    return true;
}

uint64_t getBigPrime() {
    while (true) {
        uint64_t candidate = getLowLevelPrime();
        if (MillerRabinTest(candidate)) return candidate;
    }
}
// calculate phi(n) for a given number n
int phi(uint64_t n) {
    uint64_t result = n;
    for (uint64_t i = 2; i <= sqrt(n); i++) {
        if (n % i == 0) {
            while (n % i == 0) {
                n /= i;
            }
            result -= result / i;
        }
    }
    if (n > 1) {
        result -= result / n;
    }
    return result;
}

// calculate gcd(a, b) using the Euclidean algorithm
uint64_t gcd(uint64_t a, uint64_t b) {
    if (b == 0) {
        return a;
    }
    return gcd(b, a % b);
}

// calculate a^b mod m using modular exponentiation
uint64_t modpow(uint64_t a, uint64_t b, uint64_t m) {
    uint64_t result = 1;
    while (b > 0) {
        if (b & 1) {
            result = (result * a) % m;
        }
        a = (a * a) % m;
        b >>= 1;
    }
    return result;
}

// generate a random primitive root modulo n
uint64_t generatePrimitiveRoot(uint64_t n) {
    uint64_t phiN = phi(n);
    uint64_t factors[phiN], numFactors = 0;
    uint64_t temp = phiN;
    // get all prime factors of phi(n)
    for (uint64_t i = 2; i <= sqrt(temp); i++) {
        if (temp % i == 0) {
            factors[numFactors++] = i;
            while (temp % i == 0) {
                temp /= i;
            }
        }
    }
    if (temp > 1) {
        factors[numFactors++] = temp;
    }
    // test possible primitive roots
    for (uint64_t i = 2; i <= n; i++) {
        bool isRoot = true;
        for (uint64_t j = 0; j < numFactors; j++) {
            if (modpow(i, phiN / factors[j], n) == 1) {
                isRoot = false;
                break;
            }
        }
        if (isRoot) {
            return i;
        }
    }
    return -1;
}

// Function for extended Euclidean Algorithm
uint64_t gcdExtended(uint64_t a, uint64_t b, uint64_t& x, uint64_t& y) {
    // Base Case
    if (a == 0) {
        x = 0, y = 1;
        return b;
    }

    // To store results of recursive call
    uint64_t x1, y1;
    uint64_t gcd = gcdExtended(b % a, a, x1, y1);

    // Update x and y using results of recursive
    // call
    x = y1 - (b / a) * x1;
    y = x1;

    return gcd;
}

uint64_t modInverse(uint64_t A, uint64_t M) {
    uint64_t x, y;
    uint64_t g = gcdExtended(A, M, x, y);
    if (g != 1)
        return 0;
    else
        return (x % M + M) % M;
}

int main() {
    uint64_t p = getBigPrime(), q = getBigPrime(), n = p * q,
             phiN = (p - 1) * (q - 1), e = 3, d = 2;
    cout << "p: " << p << endl
         << "q: " << q << endl
         << "n: " << n << endl
         << "phi: " << phiN << endl;
    while (gcd(e, phiN) != 1) e++;
    d = modInverse(e, phiN);
    cout << "e: " << e << endl << "d: " << d << endl;
    uint64_t msg = 16;
    uint64_t c = modpow(msg, e, n);
    uint64_t m = modpow(c, d, n);
    cout << "Original message: " << msg << endl;
    cout << "Encrypted message: " << c << endl;
    cout << "Decrypted message: " << m << endl;
    return 0;
}
