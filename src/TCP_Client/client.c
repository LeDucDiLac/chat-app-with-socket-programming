#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include "../TCP_Server/tcp_utils.h"

#define BUFF_SIZE (1 << 14)

void show_menu()
{
    printf("\nMenu:\n");
    printf("1. Log in\n");
    printf("2. Post message\n");
    printf("3. Log out\n");
    printf("4. Exit\n");
    printf("Choose an option: ");
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Invalid arguments\n");
        printf("Valid example: ./client 127.0.0.1 5550\n");
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client_sock;
    char buff[BUFF_SIZE];
    struct sockaddr_in server_addr;

    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error");
        return 1;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        printf("Invalid address: %s\n", server_ip);
        return 1;
    }

    printf("Connecting to %s:%d...\n", server_ip, server_port);
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("connect() error");
        return 1;
    }

    // Receive connection success message
    int recv_len = recv_until_delimiter(client_sock, buff, BUFF_SIZE);
    if (recv_len > 0)
    {
        printf("Server: %s\n", buff + 4);
    }
    else
    {
        printf("Failed to receive connection message\n");
        close(client_sock);
        return 1;
    }

    while (1)
    {
        show_menu();
        int choice;
        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input\n");
            while (getchar() != '\n')
                ;
            continue;
        }
        while (getchar() != '\n')
            ;

        char request[BUFF_SIZE];
        memset(request, 0, BUFF_SIZE);

        if (choice == 1)
        { // Log in
            printf("Enter username: ");
            char username[256];
            if (fgets(username, sizeof(username), stdin))
            {
                username[strcspn(username, "\n")] = '\0'; // remove newline
                sprintf(request, "USER %s\r\n", username);
            }
            else
            {
                printf("Error reading username\n");
                continue;
            }
        }
        else if (choice == 2)
        { // Post message
            printf("Enter message: ");
            char message[BUFF_SIZE - 10];
            if (fgets(message, sizeof(message), stdin))
            {
                message[strcspn(message, "\n")] = '\0';
                sprintf(request, "POST %s\r\n", message);
            }
            else
            {
                printf("Error reading message\n");
                continue;
            }
        }
        else if (choice == 3)
        { // Log out
            strcpy(request, "BYE\r\n");
        }
        else if (choice == 4)
        { // Exit
            break;
        }
        else
        {
            printf("Invalid choice\n");
            continue;
        }

        // Send request
        if (send_all(client_sock, request, strlen(request)) == -1)
        {
            perror("send error");
            break;
        }

        // Receive response
        recv_len = recv_until_delimiter(client_sock, buff, BUFF_SIZE);
        if (recv_len > 0)
        {
            printf("Server: %s\n", buff + 4);
        }
        else
        {
            printf("Failed to receive response\n");
            break;
        }
    }

    close(client_sock);
    return 0;
}
