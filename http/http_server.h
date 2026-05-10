#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lwip/sockets.h"
#include "cJSON.h"

// HTTP methods
typedef enum {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_PATCH
} http_method_t;

// HTTP status codes
typedef enum {
    HTTP_STATUS_CONTINUE = 100,
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_ACCEPTED = 202,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_MOVED_PERMANENTLY = 301,
    HTTP_STATUS_FOUND = 302,
    HTTP_STATUS_NOT_MODIFIED = 304,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_CONFLICT = 409,
    HTTP_STATUS_LENGTH_REQUIRED = 411,
    HTTP_STATUS_PAYLOAD_TOO_LARGE = 413,
    HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_GATEWAY_TIMEOUT = 504
} http_status_t;

// HTTP content types
typedef enum {
    HTTP_CONTENT_TYPE_TEXT_PLAIN,
    HTTP_CONTENT_TYPE_TEXT_HTML,
    HTTP_CONTENT_TYPE_TEXT_CSS,
    HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT,
    HTTP_CONTENT_TYPE_APPLICATION_JSON,
    HTTP_CONTENT_TYPE_APPLICATION_XML,
    HTTP_CONTENT_TYPE_APPLICATION_FORM_URLENCODED,
    HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA,
    HTTP_CONTENT_TYPE_IMAGE_PNG,
    HTTP_CONTENT_TYPE_IMAGE_JPEG,
    HTTP_CONTENT_TYPE_IMAGE_GIF,
    HTTP_CONTENT_TYPE_IMAGE_SVG,
    HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM
} http_content_type_t;

// HTTP request structure
typedef struct {
    http_method_t method;
    char uri[256];
    char query_string[256];
    char version[16];
    char headers[32][128];  // Max 32 headers
    int header_count;
    char *body;
    int body_length;
    char *raw_request;
    int raw_length;
    char *client_ip;
    uint16_t client_port;
    uint32_t timestamp;
    bool keep_alive;
    bool chunked;
    uint32_t content_length;
} http_request_t;

// HTTP response structure
typedef struct {
    http_status_t status;
    char headers[32][128];
    int header_count;
    char *body;
    int body_length;
    http_content_type_t content_type;
    char content_type_str[64];
    bool chunked;
    bool keep_alive;
    uint32_t content_length;
    char *etag;
    time_t last_modified;
    bool cache_enabled;
    uint32_t cache_max_age;
} http_response_t;

// HTTP handler function type
typedef esp_err_t (*http_handler_t)(const http_request_t *request, http_response_t *response);

// HTTP endpoint configuration
typedef struct {
    char uri[128];
    http_method_t method;
    http_handler_t handler;
    bool requires_auth;
    bool requires_https;
    uint32_t timeout_ms;
    uint32_t rate_limit;
    char *description;
} http_endpoint_t;

// HTTP client connection
typedef struct {
    int socket_fd;
    char client_ip[16];
    uint16_t client_port;
    uint32_t connect_time;
    uint32_t last_activity;
    uint32_t request_count;
    bool authenticated;
    char *auth_token;
    char *user_agent;
    bool keep_alive;
} http_client_t;

// HTTP server configuration
typedef struct {
    uint16_t port;
    uint16_t max_clients;
    uint32_t request_timeout_ms;
    uint32_t keep_alive_timeout_ms;
    uint32_t max_request_size;
    uint32_t max_response_size;
    bool enable_ssl;
    char *ssl_cert_path;
    char *ssl_key_path;
    bool enable_compression;
    bool enable_caching;
    bool enable_rate_limiting;
    uint32_t rate_limit_per_minute;
    bool enable_logging;
    char *log_file_path;
    uint8_t log_level;
} http_server_config_t;

// HTTP server statistics
typedef struct {
    uint32_t total_requests;
    uint32_t total_responses;
    uint32_t active_connections;
    uint32_t max_connections;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t requests_per_method[8]; // One for each HTTP method
    uint32_t responses_per_status[505]; // One for each status code
    uint32_t avg_request_time;
    uint32_t max_request_time;
    uint32_t min_request_time;
    uint32_t errors;
    uint32_t timeouts;
    uint32_t start_time;
    uint32_t uptime;
} http_server_stats_t;

// Function declarations
esp_err_t http_server_init(const http_server_config_t *config);
esp_err_t http_server_start(void);
esp_err_t http_server_stop(void);
esp_err_t http_server_deinit(void);
esp_err_t http_server_register_endpoint(const http_endpoint_t *endpoint);
esp_err_t http_server_unregister_endpoint(const char *uri, http_method_t method);
esp_err_t http_server_register_middleware(http_handler_t middleware);
esp_err_t http_server_set_default_handler(http_handler_t handler);
esp_err_t http_server_send_response(http_client_t *client, const http_response_t *response);
esp_err_t http_server_send_error(http_client_t *client, http_status_t status, const char *message);
esp_err_t http_server_send_file(http_client_t *client, const char *file_path, const char *mime_type);
esp_err_t http_server_send_json(http_client_t *client, cJSON *json);
esp_err_t http_server_send_redirect(http_client_t *client, const char *location, bool permanent);

// Request/response parsing and formatting
esp_err_t http_parse_request(const char *raw_request, http_request_t *request);
esp_err_t http_format_response(const http_response_t *response, char *buffer, size_t buffer_size);
esp_err_t http_parse_headers(const char *header_string, http_request_t *request);
esp_err_t http_parse_query_string(const char *query_string, char *params[][2], int max_params);
esp_err_t http_url_decode(const char *src, char *dest, size_t dest_size);
esp_err_t http_url_encode(const char *src, char *dest, size_t dest_size);

// Content type utilities
const char* http_get_content_type_string(http_content_type_t content_type);
http_content_type_t http_get_content_type_from_extension(const char *extension);
http_content_type_t http_get_content_type_from_mime(const char *mime_type);
esp_err_t http_set_content_type(http_response_t *response, http_content_type_t content_type);
esp_err_t http_set_content_type_string(http_response_t *response, const char *content_type);

// Header utilities
esp_err_t http_add_header(http_response_t *response, const char *name, const char *value);
esp_err_t http_get_header(const http_request_t *request, const char *name, char *value, size_t value_size);
bool http_has_header(const http_request_t *request, const char *name);
esp_err_t http_remove_header(http_response_t *response, const char *name);

// Authentication utilities
esp_err_t http_set_auth_header(http_response_t *response, const char *realm, const char *method);
bool http_is_authenticated(const http_request_t *request);
esp_err_t http_get_auth_token(const http_request_t *request, char *token, size_t token_size);

// Caching utilities
esp_err_t http_set_cache_headers(http_response_t *response, uint32_t max_age, bool must_revalidate);
esp_err_t http_set_etag(http_response_t *response, const char *etag);
esp_err_t http_set_last_modified(http_response_t *response, time_t last_modified);
bool http_is_not_modified(const http_request_t *request, const http_response_t *response);

// Compression utilities
esp_err_t http_compress_response(http_response_t *response);
bool http_accepts_compression(const http_request_t *request);

// Client management
http_client_t* http_client_create(int socket_fd, const char *client_ip, uint16_t client_port);
void http_client_destroy(http_client_t *client);
esp_err_t http_client_read(http_client_t *client, char *buffer, size_t buffer_size, size_t *bytes_read);
esp_err_t http_client_write(http_client_t *client, const char *data, size_t data_size);
esp_err_t http_client_close(http_client_t *client);

// Statistics and monitoring
esp_err_t http_server_get_statistics(http_server_stats_t *stats);
void http_server_print_statistics(void);
esp_err_t http_server_reset_statistics(void);
esp_err_t http_server_export_stats_json(cJSON **json);

// Utility functions
const char* http_get_method_string(http_method_t method);
const char* http_get_status_string(http_status_t status);
http_method_t http_get_method_from_string(const char *method);
http_status_t http_get_status_from_code(int code);
bool http_is_success_status(http_status_t status);
bool http_is_client_error_status(http_status_t status);
bool http_is_server_error_status(http_status_t status);

// WebSocket support (optional)
typedef struct {
    bool enabled;
    char *protocols[8];
    int protocol_count;
    uint32_t ping_interval_ms;
    uint32_t max_message_size;
} websocket_config_t;

typedef struct {
    int socket_fd;
    char client_ip[16];
    uint16_t client_port;
    char *protocol;
    bool connected;
    uint32_t connect_time;
    uint32_t last_ping;
    uint32_t messages_sent;
    uint32_t messages_received;
} websocket_client_t;

esp_err_t http_server_enable_websocket(const websocket_config_t *config);
esp_err_t websocket_send_message(websocket_client_t *client, const char *message, bool binary);
esp_err_t websocket_ping(websocket_client_t *client);
esp_err_t websocket_close(websocket_client_t *client, uint16_t code, const char *reason);

// File serving utilities
typedef struct {
    char *path;
    char *mime_type;
    bool is_directory;
    size_t file_size;
    time_t last_modified;
    char *etag;
} file_info_t;

esp_err_t http_get_file_info(const char *path, file_info_t *info);
esp_err_t http_serve_directory(http_client_t *client, const char *dir_path, const char *base_url);
esp_err_t http_serve_static_file(http_client_t *client, const char *file_path);

// Rate limiting
typedef struct {
    uint32_t requests_per_minute;
    uint32_t current_requests;
    uint32_t window_start;
    bool blocked;
} rate_limit_t;

esp_err_t http_check_rate_limit(http_client_t *client, rate_limit_t *limit);
esp_err_t http_set_rate_limit(http_client_t *client, uint32_t requests_per_minute);

// Security utilities
esp_err_t http_validate_request(const http_request_t *request);
bool http_is_safe_method(http_method_t method);
bool http_is_idempotent_method(http_method_t method);
esp_err_t http_sanitize_uri(char *uri);
esp_err_t http_check_path_traversal(const char *path);

// Debug and testing
void http_server_dump_endpoints(void);
void http_server_dump_clients(void);
esp_err_t http_server_self_test(void);
