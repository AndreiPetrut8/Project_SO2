#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 9000

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_sock, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server ON! Waiting connection...\n");

    while (1) {
        int socket_fd = accept(server_sock, NULL, NULL);
        if (socket_fd < 0) {
            perror("accept");
            continue;
        }

        printf("CLIENT CONNECTED!\n");

        struct dirent *dp;
        DIR *dir = opendir(".");
        if (dir) {
            while ((dp = readdir(dir)) != NULL) {
                if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                    continue;
                }

                struct stat file_info;
                stat(dp->d_name, &file_info);

                if (S_ISREG(file_info.st_mode)) {
                    char file_name[100];
                    snprintf(file_name, sizeof(file_name), "%s", dp->d_name);
                    write(socket_fd, file_name, strlen(file_name));
                    write(socket_fd, "\n", 1);
                }
            }
            closedir(dir);
        }
        write(socket_fd, "END\n", 4);
        close(socket_fd);
    }
}