#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// Protocol constants
#define MAX_PACKET_SIZE 65536 // 64KB max packet size
#define HEADER_SIZE 4         // 4 bytes for length prefix

// Function prototypes for sending/receiving JSON packets

/**
 * Send a JSON string through socket with length prefix
 * Packet format: [LENGTH:4bytes][JSON_PAYLOAD:variable]
 *
 * @param socket_fd Socket file descriptor
 * @param json_str JSON string to send (null-terminated)
 * @return 0 on success, -1 on error
 */
int send_json_packet(int socket_fd, const char *json_str);

/**
 * Receive a JSON packet from socket (blocking)
 * Reads length prefix first, then reads exact JSON payload
 *
 * @param socket_fd Socket file descriptor
 * @param buffer Buffer to store received JSON string
 * @param buffer_size Size of buffer
 * @return Number of bytes received (JSON length), -1 on error, 0 on connection closed
 */
int receive_json_packet(int socket_fd, char *buffer, size_t buffer_size);

#endif // PROTOCOL_H
