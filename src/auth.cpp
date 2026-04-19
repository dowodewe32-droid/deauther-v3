#include <Arduino.h>
#include "auth.h"
#include "definitions.h"

static char current_user[32] = ADMIN_USER;
static char current_pass[32] = ADMIN_PASS;
static int failed_attempts = 0;

bool validate_login(const char* username, const char* password) {
  if (failed_attempts >= MAX_LOGIN_ATTEMPTS) {
    DEBUG_PRINTLN("Too many failed login attempts");
    return false;
  }
  
  if (strcmp(username, current_user) == 0 && strcmp(password, current_pass) == 0) {
    failed_attempts = 0;
    DEBUG_PRINTLN("Login successful");
    return true;
  }
  
  failed_attempts++;
  DEBUG_PRINT("Login failed attempt ");
  DEBUG_PRINTLN(failed_attempts);
  return false;
}

void set_custom_credentials(const char* user, const char* pass) {
  strncpy(current_user, user, 31);
  current_user[31] = 0;
  strncpy(current_pass, pass, 31);
  current_pass[31] = 0;
  DEBUG_PRINTLN("Custom credentials set");
}

void reset_failed_attempts() {
  failed_attempts = 0;
}