#include "protocol.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/**
 * Send all bytes through socket (handles partial sends due to buffering)
 * This handles stream-based TCP transmission properly
 * Internal helper function - not exposed in public API
 */
static int send_all(int socket_fd, const void *data, size_t length)
{
    const char *ptr = (const char *)data;
    size_t bytes_sent = 0;

    while (bytes_sent < length)
    {
        ssize_t result = send(socket_fd, ptr + bytes_sent, length - bytes_sent, 0);
        
        if (result < 0)
        {
            if (errno == EINTR)
            {
                // Interrupted by signal, retry
                continue;
            }
            perror("send_all: send failed");
            return -1;
        }
        else if (result == 0)
        {
            // Connection closed by peer
            fprintf(stderr, "send_all: connection closed by peer\n");
            return -1;
        }

        bytes_sent += result;
    }

    return 0;
}

/**
 * Receive exact number of bytes from socket (handles partial receives)
 * This ensures we get complete data even if it arrives in multiple chunks
 * Internal helper function - not exposed in public API
 */
static int recv_all(int socket_fd, void *buffer, size_t length)
{
    char *ptr = (char *)buffer;
    size_t bytes_received = 0;

    while (bytes_received < length)
    {
        ssize_t result = recv(socket_fd, ptr + bytes_received, length - bytes_received, 0);
        
        if (result < 0)
        {
            if (errno == EINTR)
            {
                // Interrupted by signal, retry
                continue;
            }
            perror("recv_all: recv failed");
            return -1;
        }
        else if (result == 0)
        {
            // Connection closed by peer
            fprintf(stderr, "recv_all: connection closed by peer\n");
            return -2;
        }

        bytes_received += result;
    }

    return 0;
}

/**
 * Send a JSON packet with length prefix and \r\n delimiter
 * Format: [LENGTH:4bytes in network byte order][JSON string][\r\n]
 */
int send_json_packet(int socket_fd, const char *json_str)
{
    if (json_str == NULL)
    {
        fprintf(stderr, "send_json_packet: NULL json_str\n");
        return -1;
    }

    // Calculate JSON length (excluding delimiter)
    uint32_t json_length = strlen(json_str);

    // Check size limit
    if (json_length > MAX_PACKET_SIZE - HEADER_SIZE - 2)
    {
        fprintf(stderr, "send_json_packet: JSON too large (%u bytes)\n", json_length);
        return -1;
    }

    // Build complete packet: [LENGTH][JSON][\r\n] in one buffer
    char packet[MAX_PACKET_SIZE];
    
    // Add length prefix
    uint32_t network_length = htonl(json_length);
    memcpy(packet, &network_length, HEADER_SIZE);
    
    // Add JSON payload
    memcpy(packet + HEADER_SIZE, json_str, json_length);
    
    // Add delimiter
    packet[HEADER_SIZE + json_length] = '\r';
    packet[HEADER_SIZE + json_length + 1] = '\n';

    // Send entire packet in one syscall
    if (send_all(socket_fd, packet, HEADER_SIZE + json_length + 2) < 0)
    {
        fprintf(stderr, "send_json_packet: failed to send packet\n");
        return -1;
    }

    printf("[SENT] %u bytes: %s\n", json_length, json_str);
    return 0;
}

/**
 * Receive a JSON packet from socket
 * Returns number of bytes received (JSON length), -1 on error, 0 on connection closed
 */
int receive_json_packet(int socket_fd, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
    {
        fprintf(stderr, "receive_json_packet: invalid buffer\n");
        return -1;
    }

    // Receive length prefix (4 bytes)
    uint32_t network_length;
    int result = recv_all(socket_fd, &network_length, HEADER_SIZE);
    
    if (result == -2)
    {
        // Connection closed
        return 0;
    }
    else if (result < 0)
    {
        fprintf(stderr, "receive_json_packet: failed to receive length prefix\n");
        return -1;
    }

    // Convert from network byte order to host byte order
    uint32_t json_length = ntohl(network_length);

    // Validate length
    if (json_length == 0)
    {
        fprintf(stderr, "receive_json_packet: received zero length\n");
        return -1;
    }

    if (json_length > MAX_PACKET_SIZE - HEADER_SIZE - 2)
    {
        fprintf(stderr, "receive_json_packet: packet too large (%u bytes)\n", json_length);
        return -1;
    }

    if (json_length + 2 > buffer_size)
    {
        fprintf(stderr, "receive_json_packet: buffer too small (need %u, have %zu)\n", 
                json_length + 3, buffer_size);
        return -1;
    }

    // Receive JSON payload + delimiter in one syscall
    result = recv_all(socket_fd, buffer, json_length + 2);
    
    if (result == -2)
    {
        // Connection closed
        return 0;
    }
    else if (result < 0)
    {
        fprintf(stderr, "receive_json_packet: failed to receive JSON payload\n");
        return -1;
    }

    // Verify delimiter at the end
    if (buffer[json_length] != '\r' || buffer[json_length + 1] != '\n')
    {
        fprintf(stderr, "receive_json_packet: invalid delimiter (expected \\r\\n, got %02x%02x)\n",
                (unsigned char)buffer[json_length], (unsigned char)buffer[json_length + 1]);
        return -1;
    }

    // Null-terminate the JSON string
    buffer[json_length] = '\0';

    printf("[RECV] %u bytes: %s\n", json_length, buffer);
    return json_length;
}
