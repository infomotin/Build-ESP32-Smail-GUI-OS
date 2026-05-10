#include "auth.h"
#include "storage.h"
#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/md5.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "auth";
static char current_token[65] = {0};
static char session_token[65] = {0};
static uint32_t session_expiry = 0;
static const uint32_t SESSION_TIMEOUT = 3600; // 1 hour

void auth_init(void)
{
    const char *saved = storage_get_config("auth_token");
    if (saved && strlen(saved) > 0) {
        strncpy(current_token, saved, sizeof(current_token) - 1);
        ESP_LOGI(TAG, "Loaded auth token");
        return;
    }
    auth_generate_secure_token(current_token, sizeof(current_token));
    storage_save_config("auth_token", current_token);
    ESP_LOGI(TAG, "Generated secure auth token");
}

bool auth_validate_token(const char *token)
{
    return token && strcmp(token, current_token) == 0;
}

bool auth_validate_session(const char *session)
{
    if (!session || strlen(session) == 0) {
        return false;
    }
    
    uint32_t current_time = xTaskGetTickCount() / configTICK_RATE_HZ;
    if (current_time > session_expiry) {
        ESP_LOGI(TAG, "Session expired");
        memset(session_token, 0, sizeof(session_token));
        session_expiry = 0;
        return false;
    }
    
    return strcmp(session, session_token) == 0;
}

const char *auth_generate_token(void)
{
    return current_token;
}

const char *auth_create_session(const char *auth_token)
{
    if (!auth_validate_token(auth_token)) {
        ESP_LOGW(TAG, "Invalid auth token for session creation");
        return NULL;
    }
    
    auth_generate_secure_token(session_token, sizeof(session_token));
    session_expiry = (xTaskGetTickCount() / configTICK_RATE_HZ) + SESSION_TIMEOUT;
    
    ESP_LOGI(TAG, "Session created, expires in %u seconds", SESSION_TIMEOUT);
    return session_token;
}

void auth_invalidate_session(void)
{
    memset(session_token, 0, sizeof(session_token));
    session_expiry = 0;
    ESP_LOGI(TAG, "Session invalidated");
}

void auth_generate_secure_token(char *buffer, size_t buffer_size)
{
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t charset_size = sizeof(charset) - 1;
    
    uint32_t random_data[4];
    for (int i = 0; i < 4; i++) {
        random_data[i] = esp_random();
    }
    
    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
    mbedtls_md5_update(&md5_ctx, (const unsigned char*)random_data, sizeof(random_data));
    unsigned char md5_hash[16];
    mbedtls_md5_finish(&md5_ctx, md5_hash);
    mbedtls_md5_free(&md5_ctx);
    
    for (size_t i = 0; i < buffer_size - 1 && i < 32; i++) {
        buffer[i] = charset[md5_hash[i % 16] % charset_size];
    }
    buffer[buffer_size - 1] = '\0';
}
