#include <stdio.h>
#include <cstdint>
#include "openssl/sha.h"
#include "openssl/crypto.h"

int main() {
  printf("**********************************************************\n");
  printf("**********************************************************\n");
  printf("********* This is to test OpenSSL linking library ********\n");
  printf("OpenSSL Version: %s\n", OpenSSL_version(0));
  OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS |
                        OPENSSL_INIT_ADD_ALL_DIGESTS |
                        OPENSSL_INIT_LOAD_CONFIG,
                        nullptr);
  uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  uint8_t sha[32];
  SHA256(data, sizeof(data), sha);
  printf("**********************************************************\n");
  printf("**********************************************************\n");
}