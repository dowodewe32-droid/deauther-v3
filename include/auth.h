#ifndef AUTH_H
#define AUTH_H

#include <Arduino.h>

#define ADMIN_USER "admin"
#define ADMIN_PASS "Sangkur87"
#define MAX_LOGIN_ATTEMPTS 5

typedef struct {
  char username[32];
  char password[32];
} login_attempt_t;

bool validate_login(const char* username, const char* password);
void set_custom_credentials(const char* user, const char* pass);
void reset_failed_attempts();

#endif