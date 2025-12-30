#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAX_ITEMS 2048
#define CHUNK 8192

char current_path[512];
char *items[MAX_ITEMS];
int item_count = 0;
int selected = 0;

int global_sockfd;
int global_fd;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
int total_sent = 0;

struct pack {
    int offset;
    int size;
    char buffer[CHUNK];
};

void *partition_worker(void *arg) {
    int offset = *(int *)arg;
    struct pack pk;

    pk.offset = offset;
    pk.size = pread(global_fd, pk.buffer, CHUNK, offset);

    if (pk.size > 0) {
        pthread_mutex_lock(&mut);
        write(global_sockfd, &pk.offset, sizeof(pk.offset));
        write(global_sockfd, &pk.size, sizeof(pk.size));
        write(global_sockfd, pk.buffer, pk.size);
        total_sent += pk.size;
        
        mvprintw(LINES - 1, 0, "Progress: %d bytes sent...", total_sent);
        refresh();
        
        pthread_mutex_unlock(&mut);
    }
    return NULL;
}

void send_file(const char *filename) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", current_path, filename);

    struct stat st;
    if (stat(full_path, &st) < 0 || !S_ISREG(st.st_mode)) return;

    int file_size = st.st_size;
    global_fd = open(full_path, O_RDONLY);
    if (global_fd < 0) return;

    write(global_sockfd, &file_size, sizeof(file_size));

    int chunks = (file_size + CHUNK - 1) / CHUNK;
    pthread_t th[chunks];
    int offsets[chunks];
    total_sent = 0;

    for (int i = 0; i < chunks; i++) {
        offsets[i] = i * CHUNK;
        pthread_create(&th[i], NULL, partition_worker, &offsets[i]);
    }

    for (int i = 0; i < chunks; i++) {
        pthread_join(th[i], NULL);
    }

    mvprintw(LINES - 2, 0, "Transfer Complete! Closing...");
    refresh();
    sleep(2);

    close(global_fd);
    close(global_sockfd);
}

void load_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    for (int i = 0; i < item_count; i++) free(items[i]);
    item_count = 0; selected = 0;
    dir = opendir(path);
    if (!dir) return;
    while ((entry = readdir(dir)) != NULL) {
        items[item_count++] = strdup(entry->d_name);
        if (item_count >= MAX_ITEMS) break;
    }
    closedir(dir);
}

int is_directory(const char *dir, const char *name) {
    char fullpath[1024];
    struct stat st;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);
    if (stat(fullpath, &st) == -1) return 0;
    return S_ISDIR(st.st_mode);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Eroare: Lipseste descriptorul de socket.\n");
        exit(1);
    }
    global_sockfd = atoi(argv[1]);
    strcpy(current_path, "/");

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    load_directory(current_path);

    while (1) {
        clear();
        mvprintw(0, 0, "Exploring: %s", current_path);
        mvprintw(1, 0, "Press 't' to upload file, 'q' to quit");

        for (int i = 0; i < item_count; i++) {
            if (i == selected) attron(A_REVERSE);
            if (is_directory(current_path, items[i]))
                mvprintw(i + 3, 2, "[%s]", items[i]);
            else
                mvprintw(i + 3, 2, "%s", items[i]);
            if (i == selected) attroff(A_REVERSE);
        }
        refresh();

        int ch = getch();
        if (ch == KEY_UP && selected > 0) selected--;
        else if (ch == KEY_DOWN && selected < item_count - 1) selected++;
        else if (ch == 10) {
            if (is_directory(current_path, items[selected])) {
                char new_path[1024];
                if (strcmp(current_path, "/") == 0) {
                    snprintf(new_path, sizeof(new_path), "/%s", items[selected]);
                } else {
                    snprintf(new_path, sizeof(new_path), "%s/%s", current_path, items[selected]);
                }
                strcpy(current_path, new_path);
                load_directory(current_path);
            }
	    else if (!is_directory(current_path, items[selected])) {
                send_file(items[selected]);
                break;
            }
        }
        else if (ch == KEY_BACKSPACE || ch == 127) {
	  if (strcmp(current_path, "/") == 0) {
	    load_directory(current_path);
	    continue;
	  }

	  char *last = strrchr(current_path, '/');

	  if (last) {
	    if (last == current_path) {
	      strcpy(current_path, "/");
	    } else {
	      *last = '\0';
	    }
	  }

	  load_directory(current_path);
	}
        else if (ch == 'q') {
            break;
        }
    }

    endwin();
    return 0;
}
