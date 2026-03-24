#ifndef SECURITY_H
#define SECURITY_H

#include <openssl/sha.h>

int authDB(const char* password);
int noAuthDB(const char* password);
int checkAuthExists();
int verifyPassword(const char* password);

#endif // SECURITY_H
