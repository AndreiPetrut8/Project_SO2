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

///////////////////////////////////
// ---------- DEFINES ---------- //
///////////////////////////////////

#define PORT 4555
#define MAX_NAME 30
#define STORAGE "storage"
#define CHUNK 8192

//////////////////////////////////
// ---------- STRUCT ---------- //
//////////////////////////////////

typedef struct {
  int client_fd;
} THREAD_ARGS;

struct packet {
  int offset;
  int size;
  char buffer[CHUNK];
};


///////////////////////////////////////////////////////
// ---------- LIST FILES RECURSIVE WORKER ---------- //
///////////////////////////////////////////////////////

void list_files_recursive_worker(int client_fd, const char *base_path, const char *rel_path) {
    DIR *dir = opendir(base_path);
    if (!dir) return;

    struct dirent *entry;
    char buffer[1025];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        char new_rel_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, entry->d_name);
	
        if (strlen(rel_path) == 0) strcpy(new_rel_path, entry->d_name);
        else snprintf(new_rel_path, sizeof(new_rel_path), "%s/%s", rel_path, entry->d_name);

        if (entry->d_type == DT_REG) {
            snprintf(buffer, sizeof(buffer), "%s\n", new_rel_path);
            write(client_fd, buffer, strlen(buffer));
        } 
        else if (entry->d_type == DT_DIR) {
            list_files_recursive_worker(client_fd, full_path, new_rel_path);
        }
    }
    closedir(dir);
}


///////////////////////////////////////////////
// ---------- LIST DIRS RECURSIVE ---------- //
///////////////////////////////////////////////

void list_dirs_recursive(const char *base_path, const char *rel_path, char **dir_names, int *count) {
  DIR *dir = opendir(base_path);
  if (!dir || *count >= 1024) return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

      char next_base[1024];
      char next_rel[1024];
      snprintf(next_base, sizeof(next_base), "%s/%s", base_path, entry->d_name);
            
      if (strcmp(rel_path, ".") == 0)
	snprintf(next_rel, sizeof(next_rel), "%s", entry->d_name);
      else
	snprintf(next_rel, sizeof(next_rel), "%s/%s", rel_path, entry->d_name);

      dir_names[(*count)++] = strdup(next_rel);
      if (*count < 1024) {
	list_dirs_recursive(next_base, next_rel, dir_names, count);
      }
    }
  }
  closedir(dir);
}


///////////////////////////////////////////
// ---------- SEND FILES LIST ---------- //
///////////////////////////////////////////
//
// OP 2

void send_file_list(int client_fd, const char *user_dir) {
  list_files_recursive_worker(client_fd, user_dir, "");

  write(client_fd, "END\n", 4);
}


////////////////////////////////////////
// ---------- HANDLE MKDIR ---------- //
////////////////////////////////////////
//
// OP 6

void handle_mkdir(int client_fd, const char *user_dir) {
  int p_len, n_len;
  char parent[256], name[256], full_path[1024];

  read(client_fd, &p_len, sizeof(int));
  read(client_fd, parent, p_len);
  parent[p_len] = '\0';

  read(client_fd, &n_len, sizeof(int));
  read(client_fd, name, n_len);
  name[n_len] = '\0';

  if (strcmp(parent, ".") == 0)
    snprintf(full_path, sizeof(full_path), "%s/%s", user_dir, name);
  else
    snprintf(full_path, sizeof(full_path), "%s/%s/%s", user_dir, parent, name);

  if (mkdir(full_path, 0755) == 0) {
    printf("Folder creat: %s\n", full_path);
  } else {
    perror("mkdir error");
  }
}


///////////////////////////////////////////////////
// ---------- HANDLE SEND DIR TO LIST ---------- //
///////////////////////////////////////////////////
//
// OP 5

void handle_send_dir_list(int client_fd, const char *user_dir) {
  char *dir_names[1024];
  int count = 0;

  dir_names[count++] = strdup(".");

  list_dirs_recursive(user_dir, ".", dir_names, &count);

  if (write(client_fd, &count, sizeof(int)) <= 0) return;

  for (int i = 0; i < count; i++) {
    int len = strlen(dir_names[i]);
    write(client_fd, &len, sizeof(int));
    write(client_fd, dir_names[i], len);
    free(dir_names[i]);
  }
}


////////////////////////////////////////////////
// ---------- SEND AND DELETE FILE ---------- //
////////////////////////////////////////////////
//
// OP 4

void send_and_delete_file(int client_fd, const char *user_dir) {
  send_file_list(client_fd,user_dir);
  char file_name[256];
  int k = 0;
  char c;

  while (read(client_fd, &c, 1) > 0) {
    if (c == '\n') break;
    file_name[k++] = c;
  }
  file_name[k] = '\0';

  if(k == 0){
    return;
  }

  char file_path[1024];
  snprintf(file_path, sizeof(file_path), "%s/%s", user_dir, file_name);

  if (unlink(file_path) == 0) {
    printf("File deleted: %s\n", file_name);
  }
}


////////////////////////////////////////////
// ---------- SEND TO DOWNLOAD ---------- //
////////////////////////////////////////////
//
// OP 3

void send_to_download(int client_fd, const char *user_dir)
{

  send_file_list(client_fd, user_dir);


  char file_name[256];
  char c;
  int  k=0;

  while (read(client_fd, &c, 1) > 0) {
    if (c == '\n')break;
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


////////////////////////////////////////
// ---------- RECEIVE FILE ---------- //
////////////////////////////////////////
//
// OP 1

void receive_file(int sockfd, const char *user_dir)
{
  int path_len;

  if (read(sockfd, &path_len, sizeof(int)) <= 0) return;

  if(path_len == 0)return;
  
  handle_send_dir_list(sockfd, user_dir);

  if (read(sockfd, &path_len, sizeof(int)) <= 0) return;
    
  char target_subpath[512];
  read(sockfd, target_subpath, path_len);
  target_subpath[path_len] = '\0';

  int name_len;
  if (read(sockfd, &name_len, sizeof(int)) <= 0) return;

  char filename[256];
  read(sockfd, filename, name_len);
  filename[name_len] = '\0';

  int file_size;
  read(sockfd, &file_size, sizeof(int));

  char filepath[1024];
  if (strcmp(target_subpath, ".") == 0)
    snprintf(filepath, sizeof(filepath), "%s/%s", user_dir, filename);
  else
    snprintf(filepath, sizeof(filepath), "%s/%s/%s", user_dir, target_subpath, filename);

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



/////////////////////////////////////////
// ---------- HANDLE CLIENT ---------- //
/////////////////////////////////////////

void* handle_client(void *arg)
{
  THREAD_ARGS *args = (THREAD_ARGS *)arg;
  int client_fd = args->client_fd;
  free(args);

  char username[MAX_NAME] = {0};
  int name_len;

  if (read(client_fd, &name_len, sizeof(int)) <= 0) {
    close(client_fd);
    return NULL;
  }

  if (name_len <= 0 || name_len >= MAX_NAME) {
    close(client_fd);
    return NULL;
  }

  if (read(client_fd, username, name_len) <= 0) {
    close(client_fd);
    return NULL;
  }

  username[name_len] = '\0';

  int op;
  char user_dir[512];
  snprintf(user_dir, sizeof(user_dir), "%s/%s", STORAGE, username);
  struct stat st;
  if (stat(user_dir, &st) == -1) mkdir(user_dir, 0755);

  while (read(client_fd, &op, sizeof(int)) > 0) {
    printf("Comanda primita: %d pentru user: %s\n", op, username);

    if (op == 1) receive_file(client_fd, user_dir);
    else if (op == 2) send_file_list(client_fd, user_dir);
    else if (op == 3) send_to_download(client_fd, user_dir);
    else if (op == 4) send_and_delete_file(client_fd, user_dir);
    else if (op == 5) handle_send_dir_list(client_fd, user_dir);
    else if (op == 6) handle_mkdir(client_fd, user_dir);
    else break; 
  }

  close(client_fd);
  return NULL;
}

////////////////////////////////
// ---------- MAIN ---------- //
////////////////////////////////

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
