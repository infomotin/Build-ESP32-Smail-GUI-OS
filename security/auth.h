#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha256.h"

// Authentication methods
typedef enum {
    AUTH_METHOD_NONE = 0,
    AUTH_METHOD_BASIC,
    AUTH_METHOD_TOKEN,
    AUTH_METHOD_JWT,
    AUTH_METHOD_API_KEY,
    AUTH_METHOD_CERTIFICATE
} auth_method_t;

// Authentication levels
typedef enum {
    AUTH_LEVEL_NONE = 0,
    AUTH_LEVEL_GUEST,
    AUTH_LEVEL_USER,
    AUTH_LEVEL_ADMIN,
    AUTH_LEVEL_SUPER_ADMIN
} auth_level_t;

// Session states
typedef enum {
    SESSION_STATE_ACTIVE = 0,
    SESSION_STATE_EXPIRED,
    SESSION_STATE_INVALID,
    SESSION_STATE_SUSPENDED
} session_state_t;

// User structure
typedef struct {
    uint32_t id;
    char username[64];
    char password_hash[65]; // SHA-256 hash
    char salt[33];
    auth_level_t level;
    bool enabled;
    uint32_t created_time;
    uint32_t last_login_time;
    uint32_t failed_login_attempts;
    uint32_t lockout_until;
    char email[128];
    char api_key[65];
} auth_user_t;

// Session structure
typedef struct {
    char session_id[65]; // SHA-256 hash
    uint32_t user_id;
    auth_level_t level;
    uint32_t created_time;
    uint32_t last_activity_time;
    uint32_t expires_at;
    session_state_t state;
    char client_ip[16];
    char user_agent[256];
    bool remember_me;
} auth_session_t;

// Token structure
typedef struct {
    char token[65]; // SHA-256 hash
    uint32_t user_id;
    auth_level_t level;
    uint32_t created_time;
    uint32_t expires_at;
    char purpose[32];
    bool single_use;
    bool used;
} auth_token_t;

// Authentication configuration
typedef struct {
    auth_method_t method;
    uint32_t session_timeout_ms;
    uint32_t token_timeout_ms;
    uint32_t max_failed_attempts;
    uint32_t lockout_duration_ms;
    bool enable_rate_limiting;
    uint32_t rate_limit_per_minute;
    bool enable_two_factor;
    bool enable_password_complexity;
    uint8_t min_password_length;
    bool enable_session_rotation;
    uint32_t session_rotation_interval_ms;
    bool enable_logging;
    bool enable_encryption;
    uint8_t encryption_key[32];
    char jwt_secret[65];
    uint32_t max_concurrent_sessions;
} auth_config_t;

// Authentication statistics
typedef struct {
    uint32_t total_logins;
    uint32_t successful_logins;
    uint32_t failed_logins;
    uint32_t active_sessions;
    uint32_t expired_sessions;
    uint32_t locked_accounts;
    uint32_t rate_limited_requests;
    uint32_t security_violations;
    uint32_t password_changes;
    uint32_t account_creations;
    uint32_t account_deletions;
    uint32_t init_time;
    uint32_t uptime;
} auth_stats_t;

// Authentication request
typedef struct {
    char username[64];
    char password[128];
    char token[65];
    char api_key[65];
    char client_ip[16];
    char user_agent[256];
    char session_id[65];
} auth_request_t;

// Authentication response
typedef struct {
    bool success;
    auth_level_t level;
    char session_id[65];
    char token[65];
    uint32_t expires_at;
    char error_message[256];
    uint32_t retry_after;
} auth_response_t;

// Function declarations
esp_err_t auth_init(const auth_config_t *config);
esp_err_t auth_deinit(void);
esp_err_t auth_set_config(const auth_config_t *config);
esp_err_t auth_get_config(auth_config_t *config);

// User management
esp_err_t auth_create_user(const char *username, const char *password, auth_level_t level, uint32_t *user_id);
esp_err_t auth_delete_user(uint32_t user_id);
esp_err_t auth_update_user(uint32_t user_id, const char *username, const char *password, auth_level_t level);
esp_err_t auth_get_user(uint32_t user_id, auth_user_t *user);
esp_err_t auth_get_user_by_username(const char *username, auth_user_t *user);
esp_err_t auth_change_password(uint32_t user_id, const char *old_password, const char *new_password);
esp_err_t auth_reset_password(const char *username, const char *new_password);
esp_err_t auth_enable_user(uint32_t user_id, bool enabled);
esp_err_t auth_set_user_level(uint32_t user_id, auth_level_t level);

// Authentication
esp_err_t auth_authenticate(const auth_request_t *request, auth_response_t *response);
esp_err_t auth_validate_session(const char *session_id, auth_session_t *session);
esp_err_t auth_validate_token(const char *token, auth_level_t required_level, auth_token_t *auth_token);
esp_err_t auth_validate_api_key(const char *api_key, auth_level_t required_level, auth_user_t *user);
esp_err_t auth_logout(const char *session_id);
esp_err_t auth_logout_all_sessions(uint32_t user_id);

// Session management
esp_err_t auth_create_session(uint32_t user_id, auth_level_t level, const char *client_ip, 
                           const char *user_agent, bool remember_me, char *session_id);
esp_err_t auth_update_session(const char *session_id, const char *client_ip, const char *user_agent);
esp_err_t auth_extend_session(const char *session_id, uint32_t additional_time_ms);
esp_err_t auth_destroy_session(const char *session_id);
esp_err_t auth_cleanup_expired_sessions(void);
esp_err_t auth_get_session(const char *session_id, auth_session_t *session);
esp_err_t auth_list_user_sessions(uint32_t user_id, auth_session_t *sessions, size_t max_sessions, size_t *count);

// Token management
esp_err_t auth_create_token(uint32_t user_id, auth_level_t level, const char *purpose, 
                         bool single_use, uint32_t expires_at, char *token);
esp_err_t auth_validate_token_internal(const char *token, auth_token_t *auth_token);
esp_err_t auth_use_token(const char *token);
esp_err_t auth_revoke_token(const char *token);
esp_err_t auth_cleanup_expired_tokens(void);

// Security functions
esp_err_t auth_hash_password(const char *password, const char *salt, char *hash);
esp_err_t auth_generate_salt(char *salt);
esp_err_t auth_generate_session_id(char *session_id);
esp_err_t auth_generate_token(char *token);
esp_err_t auth_encrypt_data(const void *input, size_t input_len, void *output, size_t *output_len);
esp_err_t auth_decrypt_data(const void *input, size_t input_len, void *output, size_t *output_len);
bool auth_validate_password_complexity(const char *password);
bool auth_is_account_locked(const auth_user_t *user);
esp_err_t auth_lock_account(uint32_t user_id, uint32_t duration_ms);
esp_err_t auth_unlock_account(uint32_t user_id);

// Rate limiting
esp_err_t auth_check_rate_limit(const char *client_ip, uint32_t max_attempts_per_minute);
esp_err_t auth_record_login_attempt(const char *client_ip, bool success);
esp_err_t auth_cleanup_rate_limits(void);

// Logging and monitoring
esp_err_t auth_log_event(const char *event_type, uint32_t user_id, const char *client_ip, const char *details);
esp_err_t auth_get_statistics(auth_stats_t *stats);
void auth_print_statistics(void);
esp_err_t auth_reset_statistics(void);

// Utility functions
bool auth_is_valid_session_id(const char *session_id);
bool auth_is_valid_token(const char *token);
const char* auth_get_level_name(auth_level_t level);
const char* auth_get_method_name(auth_method_t method);
esp_err_t auth_get_client_info(char *client_ip, char *user_agent);

// Debug functions
void auth_dump_users(void);
void auth_dump_sessions(void);
void auth_dump_tokens(void);
esp_err_t auth_self_test(void);

// Constants
#define AUTH_MAX_USERS 32
#define AUTH_MAX_SESSIONS 64
#define AUTH_MAX_TOKENS 128
#define AUTH_SESSION_ID_LENGTH 64
#define AUTH_TOKEN_LENGTH 64
#define AUTH_PASSWORD_MIN_LENGTH 8
#define AUTH_PASSWORD_MAX_LENGTH 128
#define AUTH_SALT_LENGTH 32
#define AUTH_HASH_LENGTH 64
#define AUTH_DEFAULT_SESSION_TIMEOUT 3600000  // 1 hour
#define AUTH_DEFAULT_TOKEN_TIMEOUT 300000     // 5 minutes
#define AUTH_DEFAULT_MAX_FAILED_ATTEMPTS 5
#define AUTH_DEFAULT_LOCKOUT_DURATION 900000   // 15 minutes
#define AUTH_DEFAULT_RATE_LIMIT 10             // 10 attempts per minute
