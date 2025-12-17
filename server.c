#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define PORT 9000
#define MAX_NAME 30
#define STORAGE "storage"

typedef struct {
    int client_fd;
}THREAD_ARGS;

void* handle_client(void *arg) {
    THREAD_ARGS* args = (THREAD_ARGS *) arg;
    int client_fd = args->client_fd;
    free(args);

    printf("CLIENT CONNECTED!\n");
    write(client_fd, "LOGIN - Enter username: ", strlen("LOGIN - Enter username: "));


    char username[MAX_NAME] = {0};
    int i = 0;
    while (i < MAX_NAME-1) {
        const int n = read(client_fd, &username[i], 1);
        if (n <= 0) {
            break;
        }
        if (username[i] == '\n') {
            username[i] = '\0';
            break;
        }
        i++;
    }

    if (strlen(username) == 0) {
        write(client_fd, "ERROR - empty username\n", strlen("ERROR - empty username\n"));
        close(client_fd);
        return NULL;
    }

    char user_directory[256] = {0};
    snprintf(user_directory, sizeof(user_directory), "%s/%s", STORAGE, username);

    struct stat st;
    if (stat(user_directory, &st) == -1) {
        mkdir(user_directory, 0755);
    }

    chdir(user_directory);

    char message[256];
    snprintf(message, sizeof(message), "LOGGED IN as: %s\n", username);
    write(client_fd, message, strlen(message));

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
                char file_name[256];
                snprintf(file_name, sizeof(file_name), "%s", dp->d_name);

                int pipefd[2];

                if(pipe(pipefd)<0){
                    perror("Eroare pipe\n");
                    continue;
                }

                pid_t pid=fork();
                if(pid <0 ){
                    perror("Eroare la fork\n");
                    exit(-1);
                }
                if(pid==0){
                    close(pipefd[0]);
                    dup2(pipefd[1],1);
                    execlp("md5sum","md5sum",file_name,NULL);
                    perror("Eroare la execlp");
                    exit(-1);
                }
                else{
                    close(pipefd[1]);
                    write(client_fd, file_name, strlen(file_name));
                    write(client_fd, "\n", 1);
                    char buffer[256];
                    while( (read(pipefd[0],buffer,sizeof(buffer)-1))>0){
                        char *hash=strtok(buffer," ");
                        char aux[256];

                        snprintf(aux,sizeof(aux),"MD5 %s",hash);

                        write(client_fd,aux,strlen(aux));
                        write(client_fd, "\n", 1);
                    }
                    close(pipefd[0]);
                    waitpid(pid,NULL,0);
                    }
                }
        }
        closedir(dir);
    }
    write(client_fd, "END\n", 4);
    close(client_fd);
    printf("Client DISCONNECTED\n");

    return NULL;
}

int main() {
    struct stat st;
    if (stat(STORAGE, &st) == -1) {
        if (mkdir("storage", 0755) == -1) {
            perror("Error mkdir");
            exit(1);
        }
    }

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

    if (listen(server_sock, 10) < 0) {
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

        THREAD_ARGS *args = malloc(sizeof(THREAD_ARGS));
        args->client_fd = socket_fd;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, args) != 0) {
            perror("pthread_create");
            close(socket_fd);
            free(args);
            exit(1);
        }

        pthread_detach(client_thread);
        close(socket_fd);
        free(args);
    }

    close(server_sock);
    return 0;
}