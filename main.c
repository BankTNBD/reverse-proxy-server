#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>

// #define SERVER_PORT "8080"
#define SERVER_BACKLOG 20
#define MAX_FDS 10240
// #define REMOTE_HOST "127.0.0.1"
// #define REMOTE_PORT "80"

int connection_pair[MAX_FDS];

int create_server_socket(char *SERVER_PORT);
int connect_to_remote_host(char *REMOTE_HOST, char *REMOTE_PORT);

int main(int argc, char** argv) {
    if(argc != 4) {
        printf("Usage: %s <local_port> <remote_host> <remote_port>", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *SERVER_PORT = argv[1];
    char *REMOTE_HOST = argv[2]; 
    char *REMOTE_PORT = argv[3];

    int server_listener = create_server_socket(SERVER_PORT);
    if(server_listener == -1) {
        fprintf(stderr, "error create listening socket\n");
        exit(EXIT_FAILURE);
    }

    struct pollfd pfds[MAX_FDS];
    int fd_count = 1;

    pfds[0].fd = server_listener;
    pfds[0].events = POLLIN;

    printf("Server start at port: %s\n", SERVER_PORT);

    while(true) {
        int poll_count = poll(pfds, fd_count, -1);

        if(poll_count == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < fd_count; ++i) {
            if (pfds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                if(pfds[i].fd == server_listener) {
                    // handle new connection
                    int client_fd = accept(server_listener, NULL, NULL);
                    
                    int remote_fd = connect_to_remote_host(REMOTE_HOST, REMOTE_PORT);
                    if(remote_fd == -1) {
                        // remote host down
                        close(client_fd);
                    } else {
                        if(fd_count + 2 >= MAX_FDS) {
                            fprintf(stderr, "Proxy reached capacity.\n");
                            close(client_fd);
                            close(remote_fd);
                            continue;
                        }
                        pfds[fd_count].fd = client_fd;
                        pfds[fd_count].events = POLLIN;
                        connection_pair[client_fd] = remote_fd;
                        fd_count++;

                        pfds[fd_count].fd = remote_fd;
                        pfds[fd_count].events = POLLIN;
                        connection_pair[remote_fd] = client_fd;
                        fd_count++;
                    }
                } else {
                    // handle data transfer
                    char buffer[4096];
                    int nbytes = recv(pfds[i].fd, buffer, sizeof(buffer), 0);
                    int connect_to = connection_pair[pfds[i].fd];

                    if(nbytes <= 0) {
                        // connection close or error
                        close(pfds[i].fd);
                        close(connect_to);

                        // swap server file descriptor from back
                        // remove server file descriptor from back
                        pfds[i] = pfds[fd_count-1];
                        fd_count--;

                        // find remote file descriptor
                        for(int j = 0; j < fd_count; ++j) {
                            if(pfds[j].fd == connect_to) {
                                // swap remote file descriptor from back
                                // remove remote file descriptore from back
                                pfds[j] = pfds[fd_count-1];
                                fd_count--;
                                break;
                            }
                        }
                        i--;
                    } else {
                        int total_sent = 0;
                        while (total_sent < nbytes)
                        {
                            int sent = send(connect_to, buffer + total_sent, nbytes - total_sent, 0);
                            if (sent <= 0)
                                break;
                            total_sent += sent;
                        }
                    }
                }
            }
        }
    }
}

int create_server_socket(char *SERVER_PORT) {
    struct addrinfo hints, *res_server_info, *server_info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // use IPv4
    hints.ai_socktype = SOCK_STREAM; // use socket stream type (TCP)
    hints.ai_flags = AI_PASSIVE; // set auto fill local address

    int return_value;
    if((return_value = getaddrinfo(NULL, SERVER_PORT, &hints, &res_server_info)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(return_value));
        return -1;
    }

    int server_listener;
    // find first usable address
    for(server_info = res_server_info; server_info != NULL; server_info = server_info->ai_next) {
        // create socket
        server_listener = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
        if(server_listener == -1) {
            // perror("socket");
            continue;
        }

        int opt = 1;
        setsockopt(server_listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

        if(bind(server_listener, server_info->ai_addr, server_info->ai_addrlen) == -1) {
            close(server_listener);
            continue;
        }

        break;
    }

    if(server_info == NULL) {
        return -1;
    }

    freeaddrinfo(res_server_info);

    if(listen(server_listener, SERVER_BACKLOG) == -1) {
        return -1;
    }

    return server_listener;
    // return listener file descriptor if success, otherwise -1
}

int connect_to_remote_host(char *REMOTE_HOST, char *REMOTE_PORT) {
    struct addrinfo hints, *res_server_info, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int return_value;
    if((return_value = getaddrinfo(REMOTE_HOST, REMOTE_PORT, &hints, &res_server_info)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(return_value));
        return -1;
    }

    int server_listener;
    // find first usable address
    for (server_info = res_server_info; server_info != NULL; server_info = server_info->ai_next)
    {
        // create socket
        server_listener = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
        if (server_listener == -1)
        {
            continue;
        }

        break;
    }

    if (server_info == NULL)
    {
        return -1;
    }

    if(connect(server_listener, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        close(server_listener);
        freeaddrinfo(res_server_info);
        return -1;
    }

    freeaddrinfo(res_server_info);
    return server_listener;
}