#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#include "tcp_utils.h"

#define ACCOUNT_FILE_PATH "account.txt"
#define BACKLOG 10
#define SERVER_IP_ADDR "127.0.0.1"
#define LOG_FILE "log_20225610.txt"
#define BYE_REQUEST "BYE"
#define USER_REQUEST "USER"
#define POST_REQUEST "POST"
#define RESPONSE_SIZE (1 << 10)

/* Handler process signal*/
void sig_chld(int signo);

/* Check username in account file */
int check_username(char *username);

/* Process client request */
void process_request(int sock, char *request, int *is_logined);

/*
 * Receive and echo message to client
 * [IN] sockfd: socket descriptor that connects to client
 */

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Invalid Arguments!!!\n");
        printf("Usage: ./server Port_Number\n");
        return 0;
    }
    int server_port = atoi(argv[1]);

    int listen_sock, conn_sock; /* file descriptors */
    pid_t pid;
    struct sockaddr_in server_addr; /* server's address information */
    struct sockaddr_in client_addr; /* client's address information */
    socklen_t sin_size;

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("\nError: ");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("\nError: ");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sock, BACKLOG) == -1)
    {
        perror("\nError: ");
        exit(EXIT_FAILURE);
    }

    /* Establish a signal handler to catch SIGCHLD */
    signal(SIGCHLD, sig_chld);

    printf("Server started at port number %d\n", server_port);

    while (1)
    {

        sin_size = sizeof(struct sockaddr_in);
        if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1)
        {
            if (errno == EINTR)
                continue;
            else
            {
                perror("\nError: ");
                exit(EXIT_FAILURE);
            }
        }

        /* For each client, fork spawns a child, and the child handles the new client */
        pid = fork();

        /* fork() is called in child process */
        if (pid == 0)
        {
            int client_port;
            char client_ip[INET_ADDRSTRLEN];
            char client_request[BUFF_SIZE];
            int is_logined = 0;
            close(listen_sock);
            if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN) == NULL)
            {
                perror("inet_ntop error");
            }
            else
            {
                client_port = ntohs(client_addr.sin_port);
                printf("Got a connection from %s:%d\n", client_ip, client_port);
            }
            send_all(conn_sock, "100-Connected to the server\r\n", strlen("100-Connected to the server\r\n"));
            while (1)
            {
                memset(client_request, 0, BUFF_SIZE);
                int recv_len = recv_until_delimiter(conn_sock, client_request, BUFF_SIZE);
                if (recv_len == 0)
                {
                    printf("Client %s:%d disconnected\n", client_ip, client_port);
                    break;
                }
                else if (recv_len > 0)
                {
                    printf("Recieved from client %s:%d: %s\n", client_ip, client_port, client_request);
                    process_request(conn_sock, client_request, &is_logined);
                }
            }
            close(conn_sock);
            exit(0);
        }

        /* The parent closes the connected socket since the child handles the new client */
        close(conn_sock);
    }
    close(listen_sock);
    return 0;
}

/*
@brief The function look for username in the account.txt file

@return the status of the account
- 1: account activated
- 0: account locked
- 2: username not found
*/
int check_username(char *username)
{

    char account[1024];
    int status;
    int temp = 2;
    FILE *fptr = fopen(ACCOUNT_FILE_PATH, "r");
    if (fptr == NULL)
    {
        return 2;
    }

    while (fscanf(fptr, "%1024s %d", account, &status) == 2)
    {
        if (strcmp(account, username) == 0)
        {
            temp = status;
            break;
        }
    }
    fclose(fptr);
    return temp;
}

void sig_chld(int signo)
{
    pid_t pid;
    int stat;

    /* Wait the child process terminate */
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("Child %d terminated\n", pid);

    (void)signo; // Suppress unused parameter warning
}

void process_request(int sock, char *request, int *is_logined)
{
    char response[RESPONSE_SIZE];

    if (strcmp(request, BYE_REQUEST) == 0)
    {
        if (*is_logined == 1)
        {
            strcpy(response, "130-Logged out successfully!\r\n");
            send_all(sock, response, strlen(response));
            *is_logined = 0;
            return;
        }
        else
        {
            strcpy(response, "221-Log out FAILED, you have NOT logged in yet\r\n");
            send_all(sock, response, strlen(response));
            return;
        }
    }
    else
    {
        char type[10];
        char text[BUFF_SIZE];
        memset(type, 0, sizeof(type));
        memset(text, 0, sizeof(text));
        int scan_result = sscanf(request, "%s %s", type, text);

        if (scan_result == 2)
        {
            if (strcmp(type, USER_REQUEST) == 0)
            {
                int res = check_username(text);
                if (*is_logined == 1)
                {
                    strcpy(response, "213-Logged in FAILED, you have already logged in\r\n");
                    send_all(sock, response, strlen(response));
                    return;
                }
                else
                {
                    if (res == 1)
                    {
                        strcpy(response, "110-Logged in successfully\r\n");
                        send_all(sock, response, strlen(response));
                        *is_logined = 1;
                        return;
                    }
                    else if (res == 0)
                    {
                        strcpy(response, "211-Account is locked\r\n");
                        send_all(sock, response, strlen(response));
                        return;
                    }
                    else
                    {
                        strcpy(response, "212-Account does not exist\r\n");
                        send_all(sock, response, strlen(response));
                        return;
                    }
                }
            }
            else if (strcmp(type, POST_REQUEST) == 0)
            {
                if (*is_logined == 1)
                {
                    strcpy(response, "120-Post successful\r\n");
                    send_all(sock, response, strlen(response));
                    return;
                }
                else
                {
                    strcpy(response, "221-Post FAILED, you have NOT logged in yet\r\n");
                    send_all(sock, response, strlen(response));
                    return;
                }
            }
            else
            {
                strcpy(response, "300-Invalid request\r\n");
                send_all(sock, response, strlen(response));
                return;
            }
        }
        else
        {
            strcpy(response, "300-Invalid request\r\n");
            send_all(sock, response, strlen(response));
            return;
        }
    }
}