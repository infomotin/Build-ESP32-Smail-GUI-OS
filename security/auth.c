#include "auth.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TAG "AUTH"

// Global variables
static auth_config_t auth_config;
static auth_stats_t auth_stats;
static auth_user_t users[AUTH_MAX_USERS];
static auth_session_t sessions[AUTH_MAX_SESSIONS];
static auth_token_t tokens[AUTH_MAX_TOKENS];
static SemaphoreHandle_t auth_mutex = NULL;
static bool auth_initialized = false;
static uint32_t init_time = 0;

// Forward declarations
static esp_err_t auth_generate_salt_internal(char *salt);
static esp_err_t auth_hash_password_internal(const char *password, const char *salt, char *hash);
static esp_err_t auth_find_user_by_username(const char *username, auth_user_t **user);
static esp_err_t auth_find_session_by_id(const char *session_id, auth_session_t **session);
static esp_err_t auth_find_token_by_value(const char *token, auth_token_t **auth_token);
static void update_statistics(void);
static bool is_rate_limited(const char *client_ip);

// Initialize authentication system
esp_err_t auth_init(const auth_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "Invalid authentication configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (auth_initialized) {
        ESP_LOGW(TAG, "Authentication system already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing authentication system");
    
    // Create mutex
    auth_mutex = xSemaphoreCreateMutex();
    if (!auth_mutex) {
        ESP_LOGE(TAG, "Failed to create authentication mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Store configuration
    auth_config = *config;
    
    // Reset statistics
    memset(&auth_stats, 0, sizeof(auth_stats_t));
    init_time = esp_timer_get_time() / 1000;
    
    // Clear users, sessions, and tokens
    memset(users, 0, sizeof(users));
    memset(sessions, 0, sizeof(sessions));
    memset(tokens, 0, sizeof(tokens));
    
    // Create default admin user if no users exist
    bool has_users = false;
    for (int i = 0; i < AUTH_MAX_USERS; i++) {
        if (users[i].id != 0) {
            has_users = true;
            break;
        }
    }
    
    if (!has_users) {
        ESP_LOGI(TAG, "Creating default admin user");
        uint32_t admin_id;
        esp_err_t err = auth_create_user("admin", "admin123", AUTH_LEVEL_ADMIN, &admin_id);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create default admin user: %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGW(TAG, "Default admin user created: username='admin', password='admin123'");
    }
    
    auth_initialized = true;
    ESP_LOGI(TAG, "Authentication system initialized successfully");
    return ESP_OK;
}

// Create user
esp_err_t auth_create_user(const char *username, const char *password, auth_level_t level, uint32_t *user_id)
{
    if (!username || !password || !user_id || !auth_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(auth_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t result = ESP_OK;
    
    // Check if username already exists
    auth_user_t *existing_user;
    if (auth_find_user_by_username(username, &existing_user) == ESP_OK) {
        ESP_LOGE(TAG, "Username '%s' already exists", username);
        result = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }
    
    // Validate password complexity if enabled
    if (auth_config.enable_password_complexity && !auth_validate_password_complexity(password)) {
        ESP_LOGE(TAG, "Password does not meet complexity requirements");
        result = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }
    
    // Find free user slot
    auth_user_t *user = NULL;
    for (int i = 0; i < AUTH_MAX_USERS; i++) {
        if (users[i].id == 0) {
            user = &users[i];
            break;
        }
    }
    
    if (!user) {
        ESP_LOGE(TAG, "Maximum number of users reached");
        result = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    
    // Initialize user
    memset(user, 0, sizeof(auth_user_t));
    user->id = (user - users) + 1; // Simple ID generation
    strncpy(user->username, username, sizeof(user->username) - 1);
    user->username[sizeof(user->username) - 1] = '\0';
    user->level = level;
    user->enabled = true;
    user->created_time = esp_timer_get_time() / 1000;
    
    // Generate salt and hash password
    char salt[AUTH_SALT_LENGTH + 1];
    if (auth_generate_salt_internal(salt) != ESP_OK) {
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }
    strncpy(user->salt, salt, sizeof(user->salt));
    user->salt[sizeof(user->salt) - 1] = '\0';
    
    if (auth_hash_password_internal(password, user->salt, user->password_hash) != ESP_OK) {
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }
    
    // Generate API key
    char api_key[33];
    for (int i = 0; i < 32; i++) {
        api_key[i] = 'A' + (esp_timer_get_time() % 26);
    }
    api_key[32] = '\0';
    strncpy(user->api_key, api_key, sizeof(user->api_key));
    user->api_key[sizeof(user->api_key) - 1] = '\0';
    
    // Update statistics
    auth_stats.account_creations++;
    update_statistics();
    
    *user_id = user->id;
    
    ESP_LOGI(TAG, "User '%s' created with ID %d", username, user->id);
    
cleanup:
    xSemaphoreGive(auth_mutex);
    return result;
}

// Authenticate user
esp_err_t auth_authenticate(const auth_request_t *request, auth_response_t *response)
{
    if (!request || !response || !auth_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(auth_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t result = ESP_OK;
    memset(response, 0, sizeof(auth_response_t));
    
    // Check rate limiting
    if (auth_config.enable_rate_limiting && is_rate_limited(request->client_ip)) {
        ESP_LOGW(TAG, "Rate limit exceeded for IP %s", request->client_ip);
        response->success = false;
        strncpy(response->error_message, "Rate limit exceeded", sizeof(response->error_message) - 1);
        response->retry_after = 60000; // 1 minute
        auth_stats.rate_limited_requests++;
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }
    
    // Record login attempt
    auth_record_login_attempt(request->client_ip, false);
    
    // Find user
    auth_user_t *user;
    if (auth_find_user_by_username(request->username, &user) != ESP_OK) {
        ESP_LOGW(TAG, "User '%s' not found", request->username);
        response->success = false;
        strncpy(response->error_message, "Invalid username or password", sizeof(response->error_message) - 1);
        auth_stats.failed_logins++;
        result = ESP_ERR_NOT_FOUND;
        goto cleanup;
    }
    
    // Check if user is enabled
    if (!user->enabled) {
        ESP_LOGW(TAG, "User '%s' is disabled", request->username);
        response->success = false;
        strncpy(response->error_message, "Account is disabled", sizeof(response->error_message) - 1);
        auth_stats.failed_logins++;
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }
    
    // Check if account is locked
    if (auth_is_account_locked(user)) {
        ESP_LOGW(TAG, "User '%s' account is locked", request->username);
        response->success = false;
        strncpy(response->error_message, "Account is locked", sizeof(response->error_message) - 1);
        response->retry_after = user->lockout_until - (esp_timer_get_time() / 1000);
        auth_stats.failed_logins++;
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }
    
    // Verify password
    char hash[AUTH_HASH_LENGTH + 1];
    if (auth_hash_password_internal(request->password, user->salt, hash) != ESP_OK) {
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }
    
    if (strcmp(hash, user->password_hash) != 0) {
        ESP_LOGW(TAG, "Invalid password for user '%s'", request->username);
        response->success = false;
        strncpy(response->error_message, "Invalid username or password", sizeof(response->error_message) - 1);
        
        // Update failed attempts
        user->failed_login_attempts++;
        if (user->failed_login_attempts >= auth_config.max_failed_attempts) {
            auth_lock_account(user->id, auth_config.lockout_duration_ms);
        }
        
        auth_stats.failed_logins++;
        result = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }
    
    // Authentication successful
    response->success = true;
    response->level = user->level;
    
    // Create session
    if (auth_create_session(user->id, user->level, request->client_ip, 
                         request->user_agent, false, response->session_id) == ESP_OK) {
        response->expires_at = esp_timer_get_time() / 1000 + auth_config.session_timeout_ms;
    } else {
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }
    
    // Update user statistics
    user->last_login_time = esp_timer_get_time() / 1000;
    user->failed_login_attempts = 0;
    
    // Update global statistics
    auth_stats.successful_logins++;
    auth_stats.total_logins++;
    update_statistics();
    
    // Record successful login attempt
    auth_record_login_attempt(request->client_ip, true);
    
    ESP_LOGI(TAG, "User '%s' authenticated successfully", request->username);
    
cleanup:
    xSemaphoreGive(auth_mutex);
    return result;
}

// Validate session
esp_err_t auth_validate_session(const char *session_id, auth_session_t *session)
{
    if (!session_id || !session || !auth_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(auth_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t result = ESP_OK;
    
    // Find session
    auth_session_t *found_session;
    result = auth_find_session_by_id(session_id, &found_session);
    if (result != ESP_OK) {
        ESP_LOGD(TAG, "Session '%s' not found", session_id);
        xSemaphoreGive(auth_mutex);
        return result;
    }
    
    // Check if session is expired
    uint32_t current_time = esp_timer_get_time() / 1000;
    if (found_session->expires_at <= current_time) {
        ESP_LOGD(TAG, "Session '%s' expired", session_id);
        found_session->state = SESSION_STATE_EXPIRED;
        auth_stats.expired_sessions++;
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }
    
    // Update last activity time
    found_session->last_activity_time = current_time;
    
    // Copy session data
    *session = *found_session;
    
cleanup:
    xSemaphoreGive(auth_mutex);
    return result;
}

// Create session
esp_err_t auth_create_session(uint32_t user_id, auth_level_t level, const char *client_ip, 
                           const char *user_agent, bool remember_me, char *session_id)
{
    if (!session_id || !auth_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Find free session slot
    auth_session_t *session = NULL;
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        if (sessions[i].session_id[0] == '\0') {
            session = &sessions[i];
            break;
        }
    }
    
    if (!session) {
        ESP_LOGE(TAG, "Maximum number of sessions reached");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize session
    memset(session, 0, sizeof(auth_session_t));
    session->user_id = user_id;
    session->level = level;
    session->created_time = esp_timer_get_time() / 1000;
    session->last_activity_time = session->created_time;
    session->state = SESSION_STATE_ACTIVE;
    session->remember_me = remember_me;
    
    // Set expiration time
    uint32_t timeout = remember_me ? auth_config.session_timeout_ms * 24 : auth_config.session_timeout_ms;
    session->expires_at = session->created_time + timeout;
    
    // Copy client information
    if (client_ip) {
        strncpy(session->client_ip, client_ip, sizeof(session->client_ip) - 1);
        session->client_ip[sizeof(session->client_ip) - 1] = '\0';
    }
    
    if (user_agent) {
        strncpy(session->user_agent, user_agent, sizeof(session->user_agent) - 1);
        session->user_agent[sizeof(session->user_agent) - 1] = '\0';
    }
    
    // Generate session ID
    char raw_session_id[32];
    snprintf(raw_session_id, sizeof(raw_session_id), "%lu_%u_%lu", 
             session->created_time, user_id, esp_timer_get_time());
    
    mbedtls_md5_context_t md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts_ret(&md5_ctx);
    mbedtls_md5_update_ret(&md5_ctx, raw_session_id, strlen(raw_session_id));
    mbedtls_md5_finish_ret(&md5_ctx, (unsigned char*)session->session_id);
    mbedtls_md5_free(&md5_ctx);
    
    session->session_id[AUTH_SESSION_ID_LENGTH] = '\0';
    
    // Copy session ID to output
    strcpy(session_id, session->session_id);
    
    // Update statistics
    auth_stats.active_sessions++;
    
    ESP_LOGD(TAG, "Session created for user %d: %s", user_id, session_id);
    return ESP_OK;
}

// Hash password
static esp_err_t auth_hash_password_internal(const char *password, const char *salt, char *hash)
{
    if (!password || !salt || !hash) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char combined[AUTH_PASSWORD_MAX_LENGTH + AUTH_SALT_LENGTH + 1];
    snprintf(combined, sizeof(combined), "%s%s", password, salt);
    
    mbedtls_sha256_context_t sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts_ret(&sha256_ctx);
    mbedtls_sha256_update_ret(&sha256_ctx, combined, strlen(combined));
    mbedtls_sha256_finish_ret(&sha256_ctx, (unsigned char*)hash);
    mbedtls_sha256_free(&sha256_ctx);
    
    hash[AUTH_HASH_LENGTH] = '\0';
    return ESP_OK;
}

// Generate salt
static esp_err_t auth_generate_salt_internal(char *salt)
{
    if (!salt) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t charset_size = sizeof(charset) - 1;
    
    for (int i = 0; i < AUTH_SALT_LENGTH; i++) {
        salt[i] = charset[esp_timer_get_time() % charset_size];
    }
    
    salt[AUTH_SALT_LENGTH] = '\0';
    return ESP_OK;
}

// Find user by username
static esp_err_t auth_find_user_by_username(const char *username, auth_user_t **user)
{
    if (!username || !user) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (int i = 0; i < AUTH_MAX_USERS; i++) {
        if (users[i].id != 0 && strcmp(users[i].username, username) == 0) {
            *user = &users[i];
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

// Find session by ID
static esp_err_t auth_find_session_by_id(const char *session_id, auth_session_t **session)
{
    if (!session_id || !session) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        if (sessions[i].session_id[0] != '\0' && 
            strcmp(sessions[i].session_id, session_id) == 0) {
            *session = &sessions[i];
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

// Check if account is locked
bool auth_is_account_locked(const auth_user_t *user)
{
    if (!user) {
        return false;
    }
    
    uint32_t current_time = esp_timer_get_time() / 1000;
    return (user->lockout_until > current_time);
}

// Validate password complexity
bool auth_validate_password_complexity(const char *password)
{
    if (!password) {
        return false;
    }
    
    size_t len = strlen(password);
    if (len < auth_config.min_password_length || len > AUTH_PASSWORD_MAX_LENGTH) {
        return false;
    }
    
    bool has_upper = false;
    bool has_lower = false;
    bool has_digit = false;
    bool has_special = false;
    
    for (size_t i = 0; i < len; i++) {
        if (isupper(password[i])) has_upper = true;
        if (islower(password[i])) has_lower = true;
        if (isdigit(password[i])) has_digit = true;
        if (!isalnum(password[i])) has_special = true;
    }
    
    return has_upper && has_lower && has_digit && has_special;
}

// Update statistics
static void update_statistics(void)
{
    auth_stats.uptime = (esp_timer_get_time() / 1000) - init_time;
    
    // Count active sessions
    auth_stats.active_sessions = 0;
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        if (sessions[i].session_id[0] != '\0' && sessions[i].state == SESSION_STATE_ACTIVE) {
            auth_stats.active_sessions++;
        }
    }
    
    // Count locked accounts
    auth_stats.locked_accounts = 0;
    for (int i = 0; i < AUTH_MAX_USERS; i++) {
        if (users[i].id != 0 && auth_is_account_locked(&users[i])) {
            auth_stats.locked_accounts++;
        }
    }
}

// Print statistics
void auth_print_statistics(void)
{
    auth_stats_t stats;
    if (auth_get_statistics(&stats) == ESP_OK) {
        ESP_LOGI(TAG, "=== Authentication Statistics ===");
        ESP_LOGI(TAG, "Total logins: %d", stats.total_logins);
        ESP_LOGI(TAG, "Successful logins: %d", stats.successful_logins);
        ESP_LOGI(TAG, "Failed logins: %d", stats.failed_logins);
        ESP_LOGI(TAG, "Active sessions: %d", stats.active_sessions);
        ESP_LOGI(TAG, "Expired sessions: %d", stats.expired_sessions);
        ESP_LOGI(TAG, "Locked accounts: %d", stats.locked_accounts);
        ESP_LOGI(TAG, "Rate limited requests: %d", stats.rate_limited_requests);
        ESP_LOGI(TAG, "Security violations: %d", stats.security_violations);
        ESP_LOGI(TAG, "Password changes: %d", stats.password_changes);
        ESP_LOGI(TAG, "Account creations: %d", stats.account_creations);
        ESP_LOGI(TAG, "Account deletions: %d", stats.account_deletions);
        ESP_LOGI(TAG, "Uptime: %d seconds", stats.uptime);
        ESP_LOGI(TAG, "===============================");
    }
}

// Get statistics
esp_err_t auth_get_statistics(auth_stats_t *stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    update_statistics();
    *stats = auth_stats;
    
    return ESP_OK;
}

// Self test
esp_err_t auth_self_test(void)
{
    ESP_LOGI(TAG, "Starting authentication self-test...");
    
    esp_err_t result = ESP_OK;
    
    // Test user creation
    uint32_t test_user_id;
    esp_err_t err = auth_create_user("testuser", "TestPass123!", AUTH_LEVEL_USER, &test_user_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create test user: %s", esp_err_to_name(err));
        result = ESP_FAIL;
    }
    
    // Test authentication
    auth_request_t auth_req = {
        .username = "testuser",
        .password = "TestPass123!",
        .client_ip = "127.0.0.1",
        .user_agent = "test-agent"
    };
    
    auth_response_t auth_resp;
    err = auth_authenticate(&auth_req, &auth_resp);
    if (err != ESP_OK || !auth_resp.success) {
        ESP_LOGE(TAG, "Failed to authenticate test user");
        result = ESP_FAIL;
    }
    
    // Test session validation
    auth_session_t session;
    err = auth_validate_session(auth_resp.session_id, &session);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate test session");
        result = ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Authentication self-test %s", result == ESP_OK ? "PASSED" : "FAILED");
    return result;
}
