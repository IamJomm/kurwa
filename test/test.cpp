#include <openssl/evp.h>
#include <openssl/pem.h>

int main() {
    EVP_PKEY *pkey = EVP_PKEY_new();
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    // Generate keys
    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
    EVP_PKEY_generate(ctx, &pkey);
    // The keys are now stored in memory in the 'pkey' variable

    unsigned char msg[2048 / 8] = "Hello, World!";
    // printf("Original message: %s\n", msg);

    // Encrypt message
    unsigned char enc[2048 / 8];
    size_t enc_len;
    ctx = EVP_PKEY_CTX_new(pkey, NULL);
    EVP_PKEY_encrypt_init(ctx);
    EVP_PKEY_encrypt(ctx, enc, &enc_len, msg, strlen((char *)msg) + 1);
    // printf("Encrypted message: %s\n", enc);

    // Decrypt message
    unsigned char dec[2048 / 8];
    size_t dec_len;
    ctx = EVP_PKEY_CTX_new(pkey, NULL);
    EVP_PKEY_decrypt_init(ctx);
    EVP_PKEY_decrypt(ctx, dec, &dec_len, enc, enc_len);
    printf("Decrypted message: %s\n", dec);

    // Clean up
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);

    return 0;
}
