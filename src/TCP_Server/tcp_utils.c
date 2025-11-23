#include "tcp_utils.h"
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/**
 * Send all data through socket, handling partial sends
 * Loops until all bytes are sent or error occurs
 * @param sock: Socket file descriptor
 * @param data: Pointer to data buffer
 * @param len: Number of bytes to send
 * @return: Total bytes sent on success, -1 on error
 */
int send_all(int sock, const void *data, size_t len)
{
    size_t total_sent = 0;
    size_t bytes_left = len;
    int n;

    while (total_sent < len)
    {
        n = send(sock, (char *)data + total_sent, bytes_left, 0);
        if (n == -1)
        {
            perror("send() error");
            return -1;
        }
        total_sent += n;
        bytes_left -= n;
    }
    return total_sent;
}

/**
 * Receive exact number of bytes from socket, handling partial receives
 * Loops until all bytes received, connection closed, or error
 * @param sock: Socket file descriptor
 * @param data: Buffer to store received data
 * @param len: Exact number of bytes to receive
 * @return: Total bytes received on success, 0 if connection closed, -1 on error
 */
int recv_all(int sock, void *data, size_t len)
{
    size_t total_recv = 0;
    size_t bytes_left = len;
    int n;

    while (total_recv < len)
    {
        n = recv(sock, (char *)data + total_recv, bytes_left, 0);
        if (n == -1)
        {
            perror("recv() error");
            return -1;
        }
        if (n == 0)
        {
            // Connection closed
            return 0;
        }
        total_recv += n;
        bytes_left -= n;
    }
    return total_recv;
}

/**
 * Receive data from socket until \r\n delimiter is found
 * Uses static buffer to store leftover data between calls
 * Automatically null-terminates the message (delimiter excluded)
 * @param sock: Socket file descriptor
 * @param buffer: Buffer to store message (without \r\n)
 * @param max_len: Maximum buffer size
 * @return: Message length (excluding \r\n), 0 if connection closed, -1 on error
 */
int recv_until_delimiter(int sock, char *buffer, size_t max_len)
{
    static char leftover[BUFF_SIZE] = {0}; // Store leftover data between calls
    static int leftover_len = 0;

    int total_len = leftover_len;
    char *delimiter_pos = NULL;

    // Copy leftover data to buffer
    if (leftover_len > 0)
    {
        memcpy(buffer, leftover, leftover_len);
    }

    while (1)
    {
        // Check if we already have \r\n in buffer
        delimiter_pos = strstr(buffer, "\r\n");
        if (delimiter_pos != NULL)
        {
            // Found delimiter!
            int msg_len = delimiter_pos - buffer;

            // Calculate leftover data after \r\n
            int after_delimiter = total_len - msg_len - 2; // -2 for \r\n
            if (after_delimiter > 0)
            {
                // Save leftover for next call
                memmove(leftover, delimiter_pos + 2, after_delimiter);
                leftover_len = after_delimiter;
            }
            else
            {
                leftover_len = 0;
            }

            buffer[msg_len] = '\0'; // Null-terminate message
            return msg_len;
        }

        // Need more data, check buffer space
        if (total_len >= max_len - 1)
        {
            fprintf(stderr, "Buffer overflow: message too long\n");
            leftover_len = 0;
            return -1;
        }

        // Receive more data
        int bytes_recv = recv(sock, buffer + total_len, max_len - total_len - 1, 0);

        if (bytes_recv < 0)
        {
            perror("recv() error");
            leftover_len = 0;
            return -1;
        }

        if (bytes_recv == 0)
        {
            // Connection closed
            leftover_len = 0;
            return 0;
        }

        total_len += bytes_recv;
        buffer[total_len] = '\0';
    }
}

/**
 * Get current timestamp in dd/mm/yyyy hh:mm:ss format
 * @param buffer: Output buffer for timestamp string
 * @param size: Size of buffer (recommend at least 32 bytes)
 */
void get_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%d/%m/%Y %H:%M:%S", t);
}

/**
 * Write log entry to file with format: [timestamp]$client_addr$request$response
 * Opens file in append mode, writes one line, and closes
 * @param log_file: Path to log file
 * @param client_addr: Client address string (IP:Port)
 * @param request: Request message from client
 * @param response: Response message from server
 */
void write_log(const char *log_file, const char *client_addr, const char *request, const char *response)
{
    FILE *fp = fopen(log_file, "a");
    if (fp == NULL)
    {
        perror("Failed to open log file");
        return;
    }

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    fprintf(fp, "[%s]$%s$%s$%s\n", timestamp, client_addr, request, response);
    fclose(fp);
}
