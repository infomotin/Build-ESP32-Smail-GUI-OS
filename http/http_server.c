#include "http_server.h"
#include "scheduler/task_scheduler.h"
#include "gpio/gpio_hal.h"
#include "interrupt/interrupt_handler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "mbedtls/md5.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define TAG "HTTP_SERVER"
#define MAX_ENDPOINTS 64
#define SERVER_BACKLOG 8
#define REQUEST_BUFFER_SIZE 4096
#define RESPONSE_BUFFER_SIZE 8192
#define CLIENT_TIMEOUT_MS 30000

// Global variables
static http_endpoint_t endpoints[MAX_ENDPOINTS];
static int endpoint_count = 0;
static http_client_t *clients = NULL;
static int server_socket = -1;
static bool server_running = false;
static http_server_config_t server_config;
static http_server_stats_t server_stats;
static http_handler_t default_handler = NULL;
static http_handler_t middleware_handlers[8];
static int middleware_count = 0;
static uint32_t server_start_time = 0;

// MIME types table
static const struct {
    const char *extension;
    const char *mime_type;
} mime_types[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".txt", "text/plain"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".bin", "application/octet-stream"},
    {NULL, NULL}
};

// HTTP method strings
static const char* method_strings[] = {
    "GET", "POST", "PUT", "DELETE", "OPTIONS", "HEAD", "PATCH"
};

// HTTP status strings
static const struct {
    int code;
    const char *text;
} status_strings[] = {
    {100, "Continue"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {204, "No Content"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {304, "Not Modified"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {409, "Conflict"},
    {411, "Length Required"},
    {413, "Payload Too Large"},
    {415, "Unsupported Media Type"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"}
};

esp_err_t http_server_init(const http_server_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "Invalid HTTP server configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (server_running) {
        ESP_LOGW(TAG, "HTTP server already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Store configuration
    server_config = *config;
    
    // Initialize statistics
    memset(&server_stats, 0, sizeof(http_server_stats_t));
    server_start_time = esp_timer_get_time() / 1000;
    server_stats.start_time = server_start_time;
    
    // Clear endpoints
    memset(endpoints, 0, sizeof(endpoints));
    endpoint_count = 0;
    
    // Clear middleware handlers
    memset(middleware_handlers, 0, sizeof(middleware_handlers));
    middleware_count = 0;
    
    // Initialize client list
    clients = NULL;
    
    ESP_LOGI(TAG, "HTTP server initialized on port %d", config->port);
    return ESP_OK;
}

esp_err_t http_server_start(void)
{
    if (server_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        ESP_LOGE(TAG, "Failed to create server socket");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = server_config.request_timeout_ms / 1000;
    timeout.tv_usec = (server_config.request_timeout_ms % 1000) * 1000;
    setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_config.port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind server socket");
        close(server_socket);
        server_socket = -1;
        return ESP_ERR_INVALID_STATE;
    }
    
    // Listen for connections
    if (listen(server_socket, SERVER_BACKLOG) < 0) {
        ESP_LOGE(TAG, "Failed to listen on server socket");
        close(server_socket);
        server_socket = -1;
        return ESP_ERR_INVALID_STATE;
    }
    
    server_running = true;
    
    ESP_LOGI(TAG, "HTTP server started on port %d", server_config.port);
    return ESP_OK;
}

esp_err_t http_server_register_endpoint(const http_endpoint_t *endpoint)
{
    if (!endpoint || endpoint_count >= MAX_ENDPOINTS) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check for duplicate endpoint
    for (int i = 0; i < endpoint_count; i++) {
        if (strcmp(endpoints[i].uri, endpoint->uri) == 0 && 
            endpoints[i].method == endpoint->method) {
            ESP_LOGW(TAG, "Endpoint already registered: %s %s", 
                     http_get_method_string(endpoint->method), endpoint->uri);
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    // Add endpoint
    endpoints[endpoint_count] = *endpoint;
    endpoint_count++;
    
    ESP_LOGI(TAG, "Registered endpoint: %s %s", 
             http_get_method_string(endpoint->method), endpoint->uri);
    return ESP_OK;
}

static http_handler_t find_handler(const char *uri, http_method_t method)
{
    // Exact match first
    for (int i = 0; i < endpoint_count; i++) {
        if (strcmp(endpoints[i].uri, uri) == 0 && endpoints[i].method == method) {
            return endpoints[i].handler;
        }
    }
    
    // Wildcard match
    for (int i = 0; i < endpoint_count; i++) {
        if (strcmp(endpoints[i].uri, "*") == 0 && endpoints[i].method == method) {
            return endpoints[i].handler;
        }
    }
    
    // Default handler
    return default_handler;
}

esp_err_t http_parse_request(const char *raw_request, http_request_t *request)
{
    if (!raw_request || !request) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(request, 0, sizeof(http_request_t));
    request->raw_request = strdup(raw_request);
    request->raw_length = strlen(raw_request);
    request->timestamp = esp_timer_get_time() / 1000;
    
    char *request_copy = strdup(raw_request);
    char *line = strtok(request_copy, "\r\n");
    
    if (!line) {
        free(request_copy);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Parse request line: METHOD URI HTTP/1.1
    char method_str[16];
    char uri[256];
    char version[16];
    
    if (sscanf(line, "%15s %255s %15s", method_str, uri, version) != 3) {
        free(request_copy);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Parse method
    request->method = http_get_method_from_string(method_str);
    if (request->method == -1) {
        free(request_copy);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Parse URI and query string
    char *query = strchr(uri, '?');
    if (query) {
        *query = '\0';
        strncpy(request->uri, uri, sizeof(request->uri) - 1);
        strncpy(request->query_string, query + 1, sizeof(request->query_string) - 1);
    } else {
        strncpy(request->uri, uri, sizeof(request->uri) - 1);
    }
    
    strncpy(request->version, version, sizeof(request->version) - 1);
    
    // Parse headers
    request->header_count = 0;
    while ((line = strtok(NULL, "\r\n")) && strlen(line) > 0 && request->header_count < 32) {
        strncpy(request->headers[request->header_count], line, 127);
        request->headers[request->header_count][127] = '\0';
        request->header_count++;
    }
    
    // Parse body if present
    if (line && strlen(line) > 0) {
        request->body = strdup(line);
        request->body_length = strlen(line);
        
        // Get content length from headers
        char content_length_str[32];
        if (http_get_header(request, "Content-Length", content_length_str, sizeof(content_length_str)) == ESP_OK) {
            request->content_length = atoi(content_length_str);
        }
    }
    
    free(request_copy);
    return ESP_OK;
}

esp_err_t http_format_response(const http_response_t *response, char *buffer, size_t buffer_size)
{
    if (!response || !buffer || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int offset = 0;
    
    // Status line
    offset += snprintf(buffer + offset, buffer_size - offset, 
                     "HTTP/1.1 %d %s\r\n", response->status, 
                     http_get_status_string(response->status));
    
    // Headers
    for (int i = 0; i < response->header_count; i++) {
        offset += snprintf(buffer + offset, buffer_size - offset, 
                         "%s\r\n", response->headers[i]);
    }
    
    // Content-Type header
    if (strlen(response->content_type_str) > 0) {
        offset += snprintf(buffer + offset, buffer_size - offset, 
                         "Content-Type: %s\r\n", response->content_type_str);
    }
    
    // Content-Length header
    if (response->body && response->body_length > 0) {
        offset += snprintf(buffer + offset, buffer_size - offset, 
                         "Content-Length: %d\r\n", response->body_length);
    }
    
    // Connection header
    if (response->keep_alive) {
        offset += snprintf(buffer + offset, buffer_size - offset, 
                         "Connection: keep-alive\r\n");
    } else {
        offset += snprintf(buffer + offset, buffer_size - offset, 
                         "Connection: close\r\n");
    }
    
    // Cache headers
    if (response->cache_enabled) {
        if (response->cache_max_age > 0) {
            offset += snprintf(buffer + offset, buffer_size - offset, 
                             "Cache-Control: max-age=%d\r\n", response->cache_max_age);
        }
        
        if (response->etag) {
            offset += snprintf(buffer + offset, buffer_size - offset, 
                             "ETag: \"%s\"\r\n", response->etag);
        }
        
        if (response->last_modified > 0) {
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", 
                    gmtime(&response->last_modified));
            offset += snprintf(buffer + offset, buffer_size - offset, 
                             "Last-Modified: %s\r\n", time_str);
        }
    }
    
    // End of headers
    offset += snprintf(buffer + offset, buffer_size - offset, "\r\n");
    
    // Body
    if (response->body && response->body_length > 0) {
        int body_len = (response->body_length < buffer_size - offset) ? 
                      response->body_length : buffer_size - offset;
        memcpy(buffer + offset, response->body, body_len);
        offset += body_len;
    }
    
    buffer[offset] = '\0';
    return ESP_OK;
}

http_client_t* http_client_create(int socket_fd, const char *client_ip, uint16_t client_port)
{
    http_client_t *client = malloc(sizeof(http_client_t));
    if (!client) {
        return NULL;
    }
    
    memset(client, 0, sizeof(http_client_t));
    client->socket_fd = socket_fd;
    strncpy(client->client_ip, client_ip, sizeof(client->client_ip) - 1);
    client->client_port = client_port;
    client->connect_time = esp_timer_get_time() / 1000;
    client->last_activity = client->connect_time;
    client->keep_alive = true;
    
    // Add to client list
    client->next = clients;
    clients = client;
    
    server_stats.active_connections++;
    if (server_stats.active_connections > server_stats.max_connections) {
        server_stats.max_connections = server_stats.active_connections;
    }
    
    return client;
}

void http_client_destroy(http_client_t *client)
{
    if (!client) return;
    
    // Remove from client list
    if (clients == client) {
        clients = client->next;
    } else {
        http_client_t *current = clients;
        while (current && current->next != client) {
            current = current->next;
        }
        if (current) {
            current->next = client->next;
        }
    }
    
    // Close socket
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
    }
    
    // Free memory
    if (client->auth_token) free(client->auth_token);
    if (client->user_agent) free(client->user_agent);
    free(client);
    
    server_stats.active_connections--;
}

esp_err_t http_client_read(http_client_t *client, char *buffer, size_t buffer_size, size_t *bytes_read)
{
    if (!client || !buffer || !bytes_read) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *bytes_read = recv(client->socket_fd, buffer, buffer_size - 1, 0);
    if (*bytes_read < 0) {
        ESP_LOGE(TAG, "Failed to read from client: %d", errno);
        return ESP_ERR_INVALID_STATE;
    }
    
    buffer[*bytes_read] = '\0';
    client->last_activity = esp_timer_get_time() / 1000;
    server_stats.bytes_received += *bytes_read;
    
    return ESP_OK;
}

esp_err_t http_client_write(http_client_t *client, const char *data, size_t data_size)
{
    if (!client || !data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t total_sent = 0;
    while (total_sent < data_size) {
        ssize_t sent = send(client->socket_fd, data + total_sent, data_size - total_sent, 0);
        if (sent < 0) {
            ESP_LOGE(TAG, "Failed to write to client: %d", errno);
            return ESP_ERR_INVALID_STATE;
        }
        total_sent += sent;
    }
    
    client->last_activity = esp_timer_get_time() / 1000;
    server_stats.bytes_sent += total_sent;
    
    return ESP_OK;
}

void http_server_process_client(http_client_t *client)
{
    char request_buffer[REQUEST_BUFFER_SIZE];
    char response_buffer[RESPONSE_BUFFER_SIZE];
    http_request_t request;
    http_response_t response;
    uint32_t start_time = esp_timer_get_time() / 1000;
    
    // Receive request
    size_t bytes_read;
    esp_err_t err = http_client_read(client, request_buffer, sizeof(request_buffer), &bytes_read);
    if (err != ESP_OK || bytes_read == 0) {
        ESP_LOGW(TAG, "Failed to read request from client %s:%d", 
                 client->client_ip, client->client_port);
        return;
    }
    
    ESP_LOGD(TAG, "Received request from %s:%d:\n%s", 
              client->client_ip, client->client_port, request_buffer);
    
    // Parse request
    err = http_parse_request(request_buffer, &request);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to parse request: %s", esp_err_to_name(err));
        http_server_send_error(client, HTTP_STATUS_BAD_REQUEST, "Invalid request");
        return;
    }
    
    // Set client info in request
    request.client_ip = client->client_ip;
    request.client_port = client->client_port;
    
    // Update statistics
    server_stats.total_requests++;
    if (request.method < 8) {
        server_stats.requests_per_method[request.method]++;
    }
    
    // Initialize response
    memset(&response, 0, sizeof(http_response_t));
    response.status = HTTP_STATUS_OK;
    response.keep_alive = request.keep_alive;
    response.cache_enabled = true;
    response.cache_max_age = 3600; // 1 hour default
    
    // Find and execute handler
    http_handler_t handler = find_handler(request.uri, request.method);
    if (handler) {
        err = handler(&request, &response);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Handler failed: %s", esp_err_to_name(err));
            response.status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
            
            // Clean up previous response body
            if (response.body) {
                free(response.body);
                response.body = NULL;
            }
            
            response.body = strdup("Internal Server Error");
            response.body_length = strlen(response.body);
        }
    } else {
        response.status = HTTP_STATUS_NOT_FOUND;
        response.body = strdup("Not Found");
        response.body_length = strlen(response.body);
    }
    
    // Format response
    err = http_format_response(&response, response_buffer, sizeof(response_buffer));
    if (err == ESP_OK) {
        // Send response
        err = http_client_write(client, response_buffer, strlen(response_buffer));
        if (err == ESP_OK) {
            ESP_LOGD(TAG, "Sent response to %s:%d:\n%s", 
                      client->client_ip, client->client_port, response_buffer);
        }
    }
    
    // Update statistics
    server_stats.total_responses++;
    if (response.status < 505) {
        server_stats.responses_per_status[response.status]++;
    }
    
    uint32_t request_time = (esp_timer_get_time() / 1000) - start_time;
    if (request_time > server_stats.max_request_time) {
        server_stats.max_request_time = request_time;
    }
    if (server_stats.min_request_time == 0 || request_time < server_stats.min_request_time) {
        server_stats.min_request_time = request_time;
    }
    server_stats.avg_request_time = 
        (server_stats.avg_request_time * 9 + request_time) / 10;
    
    // Clean up
    if (request.raw_request) free(request.raw_request);
    if (request.body) free(request.body);
    if (response.body) free(response.body);
    if (response.etag) free(response.etag);
}

esp_err_t http_server_send_error(http_client_t *client, http_status_t status, const char *message)
{
    if (!client) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char error_html[512];
    snprintf(error_html, sizeof(error_html),
             "<html><head><title>Error %d</title></head>"
             "<body><h1>Error %d: %s</h1>"
             "<p>%s</p></body></html>",
             status, status, http_get_status_string(status), 
             message ? message : http_get_status_string(status));
    
    http_response_t response = {
        .status = status,
        .body = error_html,
        .body_length = strlen(error_html),
        .content_type = HTTP_CONTENT_TYPE_TEXT_HTML,
        .keep_alive = false
    };
    
    strcpy(response.content_type_str, "text/html");
    
    char response_buffer[2048];
    esp_err_t err = http_format_response(&response, response_buffer, sizeof(response_buffer));
    if (err == ESP_OK) {
        return http_client_write(client, response_buffer, strlen(response_buffer));
    }
    
    return err;
}

esp_err_t http_server_send_json(http_client_t *client, cJSON *json)
{
    if (!client || !json) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char *json_string = cJSON_PrintUnformatted(json);
    if (!json_string) {
        return ESP_ERR_NO_MEM;
    }
    
    http_response_t response = {
        .status = HTTP_STATUS_OK,
        .body = json_string,
        .body_length = strlen(json_string),
        .content_type = HTTP_CONTENT_TYPE_APPLICATION_JSON,
        .keep_alive = true,
        .cache_enabled = false
    };
    
    strcpy(response.content_type_str, "application/json");
    
    char response_buffer[4096];
    esp_err_t err = http_format_response(&response, response_buffer, sizeof(response_buffer));
    if (err == ESP_OK) {
        err = http_client_write(client, response_buffer, strlen(response_buffer));
    }
    
    free(json_string);
    return err;
}

const char* http_get_method_string(http_method_t method)
{
    if (method >= 0 && method < sizeof(method_strings)/sizeof(method_strings[0])) {
        return method_strings[method];
    }
    return "UNKNOWN";
}

const char* http_get_status_string(http_status_t status)
{
    for (int i = 0; i < sizeof(status_strings)/sizeof(status_strings[0]); i++) {
        if (status_strings[i].code == status) {
            return status_strings[i].text;
        }
    }
    return "Unknown";
}

http_method_t http_get_method_from_string(const char *method)
{
    if (!method) return -1;
    
    for (int i = 0; i < sizeof(method_strings)/sizeof(method_strings[0]); i++) {
        if (strcasecmp(method, method_strings[i]) == 0) {
            return i;
        }
    }
    return -1;
}

esp_err_t http_get_header(const http_request_t *request, const char *name, char *value, size_t value_size)
{
    if (!request || !name || !value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (int i = 0; i < request->header_count; i++) {
        char *colon = strchr(request->headers[i], ':');
        if (colon) {
            *colon = '\0';
            if (strcasecmp(request->headers[i], name) == 0) {
                strncpy(value, colon + 2, value_size - 1); // Skip ": "
                value[value_size - 1] = '\0';
                *colon = ':'; // Restore colon
                return ESP_OK;
            }
            *colon = ':'; // Restore colon
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t http_add_header(http_response_t *response, const char *name, const char *value)
{
    if (!response || !name || !value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (response->header_count >= 32) {
        return ESP_ERR_NO_MEM;
    }
    
    snprintf(response->headers[response->header_count], 127, "%s: %s", name, value);
    response->headers[response->header_count][127] = '\0';
    response->header_count++;
    
    return ESP_OK;
}

const char* http_get_content_type_string(http_content_type_t content_type)
{
    switch (content_type) {
        case HTTP_CONTENT_TYPE_TEXT_PLAIN: return "text/plain";
        case HTTP_CONTENT_TYPE_TEXT_HTML: return "text/html";
        case HTTP_CONTENT_TYPE_TEXT_CSS: return "text/css";
        case HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT: return "application/javascript";
        case HTTP_CONTENT_TYPE_APPLICATION_JSON: return "application/json";
        case HTTP_CONTENT_TYPE_APPLICATION_XML: return "application/xml";
        case HTTP_CONTENT_TYPE_APPLICATION_FORM_URLENCODED: return "application/x-www-form-urlencoded";
        case HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA: return "multipart/form-data";
        case HTTP_CONTENT_TYPE_IMAGE_PNG: return "image/png";
        case HTTP_CONTENT_TYPE_IMAGE_JPEG: return "image/jpeg";
        case HTTP_CONTENT_TYPE_IMAGE_GIF: return "image/gif";
        case HTTP_CONTENT_TYPE_IMAGE_SVG: return "image/svg+xml";
        case HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM: return "application/octet-stream";
        default: return "text/plain";
    }
}

http_content_type_t http_get_content_type_from_extension(const char *extension)
{
    if (!extension) return HTTP_CONTENT_TYPE_TEXT_PLAIN;
    
    for (int i = 0; mime_types[i].extension; i++) {
        if (strcasecmp(extension, mime_types[i].extension) == 0) {
            // Find matching content type enum
            const char *mime = mime_types[i].mime_type;
            if (strcmp(mime, "text/plain") == 0) return HTTP_CONTENT_TYPE_TEXT_PLAIN;
            if (strcmp(mime, "text/html") == 0) return HTTP_CONTENT_TYPE_TEXT_HTML;
            if (strcmp(mime, "text/css") == 0) return HTTP_CONTENT_TYPE_TEXT_CSS;
            if (strcmp(mime, "application/javascript") == 0) return HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT;
            if (strcmp(mime, "application/json") == 0) return HTTP_CONTENT_TYPE_APPLICATION_JSON;
            if (strcmp(mime, "application/xml") == 0) return HTTP_CONTENT_TYPE_APPLICATION_XML;
            if (strcmp(mime, "image/png") == 0) return HTTP_CONTENT_TYPE_IMAGE_PNG;
            if (strcmp(mime, "image/jpeg") == 0) return HTTP_CONTENT_TYPE_IMAGE_JPEG;
            if (strcmp(mime, "image/gif") == 0) return HTTP_CONTENT_TYPE_IMAGE_GIF;
            if (strcmp(mime, "image/svg+xml") == 0) return HTTP_CONTENT_TYPE_IMAGE_SVG;
            if (strcmp(mime, "application/octet-stream") == 0) return HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM;
        }
    }
    
    return HTTP_CONTENT_TYPE_TEXT_PLAIN;
}

esp_err_t http_server_get_statistics(http_server_stats_t *stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *stats = server_stats;
    stats->uptime = (esp_timer_get_time() / 1000) - server_start_time;
    
    return ESP_OK;
}

void http_server_print_statistics(void)
{
    http_server_stats_t stats;
    http_server_get_statistics(&stats);
    
    ESP_LOGI(TAG, "=== HTTP Server Statistics ===");
    ESP_LOGI(TAG, "Uptime: %d seconds", stats.uptime);
    ESP_LOGI(TAG, "Total requests: %d", stats.total_requests);
    ESP_LOGI(TAG, "Total responses: %d", stats.total_responses);
    ESP_LOGI(TAG, "Active connections: %d", stats.active_connections);
    ESP_LOGI(TAG, "Max connections: %d", stats.max_connections);
    ESP_LOGI(TAG, "Bytes sent: %d", stats.bytes_sent);
    ESP_LOGI(TAG, "Bytes received: %d", stats.bytes_received);
    ESP_LOGI(TAG, "Avg request time: %d ms", stats.avg_request_time);
    ESP_LOGI(TAG, "Min request time: %d ms", stats.min_request_time);
    ESP_LOGI(TAG, "Max request time: %d ms", stats.max_request_time);
    ESP_LOGI(TAG, "Errors: %d", stats.errors);
    ESP_LOGI(TAG, "Timeouts: %d", stats.timeouts);
    
    // Request methods
    ESP_LOGI(TAG, "Requests by method:");
    for (int i = 0; i < 8; i++) {
        if (stats.requests_per_method[i] > 0) {
            ESP_LOGI(TAG, "  %s: %d", http_get_method_string(i), stats.requests_per_method[i]);
        }
    }
    
    // Response status codes
    ESP_LOGI(TAG, "Responses by status:");
    for (int i = 0; i < 505; i++) {
        if (stats.responses_per_status[i] > 0) {
            ESP_LOGI(TAG, "  %d %s: %d", i, http_get_status_string(i), stats.responses_per_status[i]);
        }
    }
    
    ESP_LOGI(TAG, "================================");
}
