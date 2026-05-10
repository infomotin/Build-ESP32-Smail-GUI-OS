#pragma once

#include <stddef.h>

void auth_init(void);
bool auth_validate_token(const char *token);
bool auth_validate_session(const char *session);
const char *auth_generate_token(void);
const char *auth_create_session(const char *auth_token);
void auth_invalidate_session(void);
void auth_generate_secure_token(char *buffer, size_t buffer_size);
