#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>


#define PORT 4555
#define MAX_NAME 30
#define STORAGE "storage"
#define CHUNK 8192

typedef struct {
    int client_fd;
} THREAD_ARGS;

struct packet {
    int offset;
    int size;
    char buffer[CHUNK];
};


void receive_file(int sockfd, const char *user_dir)
{
    int name_len;

    if (read(sockfd, &name_len, sizeof(int)) <= 0)
        return;

    if (name_len <= 0 || name_len > 255)
        return;

    char filename[256];
    if (read(sockfd, filename, name_len) <= 0)
        return;
    filename[name_len] = '\0';

    int file_size;
    if (read(sockfd, &file_size, sizeof(int)) <= 0)
        return;

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", user_dir, filename);

    int fd = open(filepath, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open output file");
        return;
    }

    int received = 0;
    struct packet pk;

    while (received < file_size) {
        if (read(sockfd, &pk.offset, sizeof(pk.offset)) <= 0) break;
        if (read(sockfd, &pk.size, sizeof(pk.size)) <= 0) break;
        if (read(sockfd, pk.buffer, pk.size) <= 0) break;

        lseek(fd, pk.offset, SEEK_SET);
        write(fd, pk.buffer, pk.size);
        received += pk.size;
    }

    close(fd);
    printf("Received file: %s (%d bytes)\n", filename, file_size);
}




void send_file_list(int client_fd, const char *user_dir) {
    DIR *dir = opendir(user_dir);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char buffer[1024];

    while ((entry = readdir(dir)) != NULL) {
      
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(buffer, sizeof(buffer), "%s\n", entry->d_name);
        write(client_fd, buffer, strlen(buffer));
    }

    

    write(client_fd, "END\n", 4);

    closedir(dir);
}


void send_and_delete_file(int client_fd, const char *user_dir) {
    send_file_list(client_fd,user_dir);
    char file_name[256];
    int k = 0;
    char c;

    // citire nume fișier până la \n
    while (read(client_fd, &c, 1) > 0) {
        if (c == '\n') break;
        file_name[k++] = c;
    }
    file_name[k] = '\0';

    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s/%s", user_dir, file_name);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("open file for send");
        return;
    }

    struct stat st;
    fstat(fd, &st);
    int file_size = st.st_size;

    write(client_fd, &file_size, sizeof(int));

    char buffer[1024];
    int bytes;
    while ((bytes = read(fd, buffer, 1024)) > 0) {
        write(client_fd, buffer, bytes);
    }
    close(fd);

    if (unlink(file_path) == 0) {
        printf("File deleted: %s\n", file_name);
    } 
}

void send_to_download(int client_fd, const char *user_dir)
{
    
    send_file_list(client_fd, user_dir);

    
    char file_name[256];
    char c;
    int  k=0;

    while (read(client_fd, &c, 1) > 0) {
        if (c == '\n')
            break;
        file_name[k++] = c;
    }
    file_name[k] = '\0';

    if (k == 0) return;

    
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s/%s", user_dir, file_name);

    
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("open download file");
        return;
    }

    
    struct stat st;
    fstat(fd, &st);
    int file_size = st.st_size;

    write(client_fd, &file_size, sizeof(int));

    
    char buffer[CHUNK];
    int bytes;

    while ((bytes = read(fd, buffer, CHUNK)) > 0) {
        write(client_fd, buffer, bytes);
    }

    close(fd);

    printf("Download sent: %s (%d bytes)\n", file_name, file_size);
}




    void* handle_client(void *arg)
{
    THREAD_ARGS *args = (THREAD_ARGS *)arg;
    int client_fd = args->client_fd;
    free(args);

    char username[MAX_NAME] = {0};
    int name_len;

    
    if (read(client_fd, &name_len, sizeof(int)) <= 0) { close(client_fd); return NULL; }
    if (name_len <= 0 || name_len >= MAX_NAME) { close(client_fd); return NULL; }
    if (read(client_fd, username, name_len) <= 0) { close(client_fd); return NULL; }
    username[name_len] = '\0';

   int op;
   read(client_fd,&op,sizeof(int));
   printf("OP>>>>>>%d\n",op);
    char user_dir[512];
    snprintf(user_dir, sizeof(user_dir), "%s/%s", STORAGE, username);
    struct stat st;
    if (stat(user_dir, &st) == -1) mkdir(user_dir, 0755);

    //Se trimite EOF din shutdown din client
    while (1) {

        if(op==1){
        receive_file(client_fd, user_dir);
    }
    if(op==2){
        send_file_list(client_fd, user_dir);
    }
    if(op==3){
        send_to_download(client_fd,user_dir);
    }
    if(op==4){
        send_and_delete_file(client_fd,user_dir);
    }
}

    close(client_fd);
    return NULL;
}


int main(void)
{
    struct stat st;
    if (stat(STORAGE, &st) == -1) {
        mkdir(STORAGE, 0755);
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 10);

    printf("Server ON (port %d)\n", PORT);

    while (1) {
        int client_fd = accept(server_sock, NULL, NULL);

        THREAD_ARGS *args = malloc(sizeof(THREAD_ARGS));
        args->client_fd = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, args);
        pthread_detach(tid);
    }
}
