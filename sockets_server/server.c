#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <sys/epoll.h>

#define UNIX_SOCKET_PATH "./volume/unix_socket"
#define INET_PORT 8080
#define BUFFER_SIZE 1024

int server_fd, client_fd;
FILE *file_ptr;
clock_t timer;

void start_timer() {
    timer = clock();
}

void stop_timer() {
    timer = clock() - timer;
    fprintf(file_ptr, "Time elapsed: %.6f seconds\n", ((double)timer)/CLOCKS_PER_SEC);
}

void handle_client(int packet_size, int num_packets) {
    char buffer[packet_size];
    ssize_t bytes_received;
    int total_bytes_received = 0;
    int goal_bytes = num_packets * packet_size;

    start_timer();

    while (total_bytes_received < goal_bytes) {
        bytes_received = recv(client_fd, buffer, packet_size, 0);
        //fprintf(file_ptr, "Received bytes: %ld\n", bytes_received);
        if (bytes_received > 0) {
            //send(client_fd, buffer, bytes_received, 0);
            total_bytes_received += bytes_received;
        } else if (bytes_received == 0) {
            break;
        } else if (bytes_received == -1 && errno != EAGAIN) {
            perror("recv");
            break;
        }
    }

    stop_timer();
    close(client_fd);
}

void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void run_epoll(int blocking, int packet_size, int num_packets) {
    struct epoll_event event, events[10];
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int n = epoll_wait(epoll_fd, events, 10, -1);
        if (n == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                client_fd = accept(server_fd, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                printf("Client connected\n");
                
                if (blocking) {
                    handle_client(packet_size, num_packets);
                    printf("Client disconnected\n");
                } else {
                    set_non_blocking(client_fd);
                    handle_client(packet_size, num_packets);
                    printf("Client disconnected\n");
                }
            } else {
                handle_client(packet_size, num_packets);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    file_ptr = fopen("./volume/logs.txt", "a");
    setbuf(file_ptr, NULL);
    int socket_type = AF_INET;
    int mode = 0;
    int packet_size = BUFFER_SIZE;
    int num_packets = 10;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-mode") == 0) {
            if (strcmp(argv[i + 1], "inet_sync_blocking") == 0) {
                socket_type = AF_INET;
                mode = 0;
            } else if (strcmp(argv[i + 1], "inet_sync_nonblocking") == 0) {
                socket_type = AF_INET;
                mode = 1;
            } else if (strcmp(argv[i + 1], "inet_async_blocking") == 0) {
                socket_type = AF_INET;
                mode = 2;
            } else if (strcmp(argv[i + 1], "inet_async_nonblocking") == 0) {
                socket_type = AF_INET;
                mode = 3;
            } else if (strcmp(argv[i + 1], "unix_sync_blocking") == 0) {
                socket_type = AF_UNIX;
                mode = 0;
            } else if (strcmp(argv[i + 1], "unix_sync_nonblocking") == 0) {
                socket_type = AF_UNIX;
                mode = 1;
            } else if (strcmp(argv[i + 1], "unix_async_blocking") == 0) {
                socket_type = AF_UNIX;
                mode = 2;
            } else if (strcmp(argv[i + 1], "unix_async_nonblocking") == 0) {
                socket_type = AF_UNIX;
                mode = 3;
            } else {
                fprintf(stderr, "Invalid mode\n");
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(argv[i], "-packet_size") == 0 && i + 1 < argc) {
            packet_size = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-num_packets") == 0 && i + 1 < argc) {
            num_packets = atoi(argv[i + 1]);
        }
    }

    if (socket_type == AF_INET) {
        struct sockaddr_in addr_in;
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        memset(&addr_in, 0, sizeof(addr_in));
        addr_in.sin_family = AF_INET;
        addr_in.sin_addr.s_addr = INADDR_ANY;
        addr_in.sin_port = htons(INET_PORT);

        if (bind(server_fd, (struct sockaddr *)&addr_in, sizeof(addr_in)) == -1) {
            perror("bind");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 5) == -1) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        printf("INET server listening on port %d...\n", INET_PORT);

    } else {
        struct sockaddr_un addr_un;
        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        memset(&addr_un, 0, sizeof(addr_un));
        addr_un.sun_family = AF_UNIX;
        strncpy(addr_un.sun_path, UNIX_SOCKET_PATH, sizeof(addr_un.sun_path) - 1);
        unlink(UNIX_SOCKET_PATH);

        if (bind(server_fd, (struct sockaddr *)&addr_un, sizeof(addr_un)) == -1) {
            perror("bind");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 5) == -1) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        printf("UNIX server listening on %s...\n", UNIX_SOCKET_PATH);
    }

    if (mode == 0) {
        while (1) {
            client_fd = accept(server_fd, NULL, NULL);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }

            printf("Client connected\n");
            handle_client(packet_size, num_packets);
            printf("Client disconnected\n");
        }
    } else if (mode == 1) {
        set_non_blocking(server_fd);
        while (1) {
            client_fd = accept(server_fd, NULL, NULL);
            if (client_fd != -1) {
                printf("Client connected\n");
                handle_client(packet_size, num_packets);
                printf("Client disconnected\n");
            }
            //usleep(1000);
        }
    } else if (mode == 2 || mode == 3) {
        run_epoll(mode == 2, packet_size, num_packets); 
    }

    close(server_fd);
    return 0;
}
