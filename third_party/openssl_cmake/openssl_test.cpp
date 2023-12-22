#include <stdio.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#define APP_OPENSSL_MAJOR_VERSION (3)
#define APP_OPENSSL_MINOR_VERSION (0)
#define OPENSSL_API_COMPAT \
  (APP_OPENSSL_MAJOR_VERSION * 10000 + APP_OPENSSL_MINOR_VERSION * 100)
#include "openssl/configuration.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"

int main() {
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
  const char* expected_sha =
      "\x66\x84\x0d\xda\x15\x4e\x8a\x11\x3c\x31\xdd\x0a\xd3\x2f\x7f\x3a\x36\x6a"
      "\x80\xe8\x13\x69\x79\xd8\xf5\xa1\x01\xd3\xd2\x9d\x6f\x72";
  int status = memcmp(sha, expected_sha, sizeof(sha));
  printf("SHA256 status: %d\n", status);
  assert(status == 0);
  printf("**********************************************************\n");
  return status;
}