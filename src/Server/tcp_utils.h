#ifndef TCP_UTILS_H
#define TCP_UTILS_H

#include <stddef.h>
#include <stdint.h>

#define BUFF_SIZE (1 << 14)
/**
 * Send all data, handling partial sends
 * Returns: total bytes sent on success, -1 on error
 */
int send_all(int sock, const void *data, size_t len);

/**
 * Receive messages until \r\n delimiter is found
 * Returns: number of bytes in message (excluding \r\n), -1 on error, 0 on connection close
 */
int recv_until_delimiter(int sock, char *buffer, size_t max_len);

/**
 * Receive exact number of bytes (handles partial receives)
 * Returns: total bytes received on success, 0 on connection close, -1 on error
 */
int recv_all(int sock, void *data, size_t len);

/**
 * Write log entry to log file
 * Format: [dd/mm/yyyy hh:mm:ss]$client_addr$request$response
 */
void write_log(const char *log_file, const char *client_addr, const char *request, const char *response);

/**
 * Get current timestamp in format dd/mm/yyyy hh:mm:ss
 */
void get_timestamp(char *buffer, size_t size);

#endif // TCP_UTILS_H
