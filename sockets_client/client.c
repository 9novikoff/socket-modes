#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define UNIX_SOCKET_PATH "/tmp/unix_socket"
#define INET_PORT 8080

void run_client(int socket_type, int packet_size, int num_packets) {
    int sock_fd;
    char *buffer = (char *)malloc(packet_size);
    memset(buffer, 'A', packet_size);

    if (socket_type == AF_INET) {
        struct sockaddr_in addr_in;
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        addr_in.sin_family = AF_INET;
        addr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr_in.sin_port = htons(INET_PORT);

        if (connect(sock_fd, (struct sockaddr *)&addr_in, sizeof(addr_in)) == -1) {
            perror("connect");
            exit(EXIT_FAILURE);
        }
    } else {
        struct sockaddr_un addr_un;
        sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        memset(&addr_un, 0, sizeof(addr_un));
        addr_un.sun_family = AF_UNIX;
        strncpy(addr_un.sun_path, UNIX_SOCKET_PATH, sizeof(addr_un.sun_path) - 1);

        if (connect(sock_fd, (struct sockaddr *)&addr_un, sizeof(addr_un)) == -1) {
            perror("connect");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_packets; i++) {
        send(sock_fd, buffer, packet_size, 0);
    }
    printf("Sent %d packets of size %d\n", num_packets, packet_size);

    // for (int i = 0; i < num_packets; i++) {
    //     char ack[packet_size];
    //     ssize_t bytes_received = recv(sock_fd, ack, packet_size, 0);
    //     if (bytes_received > 0) {
    //         // Process acknowledgment if needed
    //     }
    // }

    close(sock_fd);
    free(buffer);
}

int main(int argc, char *argv[]) {
    int socket_type;
    int packet_size = BUFFER_SIZE;
    int num_packets = 10;

    if (argc < 2) {
        printf("No mode provided. Defaulting to inet\n");
        socket_type = AF_INET;
    } else if (strcmp(argv[1], "-mode") == 0) {
        if (strcmp(argv[2], "inet") == 0) {
            socket_type = AF_INET;
        } else if (strcmp(argv[2], "unix") == 0) {
            socket_type = AF_UNIX;
        } else {
            fprintf(stderr, "Invalid mode\n");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-packet_size") == 0 && i + 1 < argc) {
            packet_size = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-num_packets") == 0 && i + 1 < argc) {
            num_packets = atoi(argv[i + 1]);
        }
    }

    run_client(socket_type, packet_size, num_packets);

    return 0;
}
