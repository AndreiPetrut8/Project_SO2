#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

#define MAX_FILES 1024
#define BUFFER_SIZE 8192

char *file_list[MAX_FILES];
int file_count = 0;
int selected = 0;
int global_sockfd;

void download_selected_file(const char *filename) {
    write(global_sockfd, filename, strlen(filename));
    write(global_sockfd, "\n", 1);

    int file_size;
    if (read(global_sockfd, &file_size, sizeof(int)) <= 0) return;

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return;

    int total_received = 0;
    char buffer[BUFFER_SIZE];
    
    while (total_received < file_size) {
        int to_read = (file_size - total_received > BUFFER_SIZE) ? BUFFER_SIZE : (file_size - total_received);
        int n = read(global_sockfd, buffer, to_read);
        if (n <= 0) break;
        
        write(fd, buffer, n);
        total_received += n;

        mvprintw(LINES - 1, 0, "Downloading: %d/%d bytes", total_received, file_size);
        refresh();
    }

    close(fd);
    mvprintw(LINES - 2, 0, "Fisierul %s a fost salvat local!", filename);
    refresh();
    sleep(2);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        return 1;
    }

    global_sockfd = atoi(argv[1]);

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    char temp[1024];
    int idx = 0;
    while (file_count < MAX_FILES) {
        char c;
        if (read(global_sockfd, &c, 1) <= 0) break;
        if (c == '\n') {
            temp[idx] = '\0';
            if (strcmp(temp, "END") == 0) break;
            file_list[file_count++] = strdup(temp);
            idx = 0;
        } else {
            temp[idx++] = c;
        }
    }

    while (1) {
        clear();
        mvprintw(0, 0, "Server Files (Select with ENTER, 'q' to go back):");
        for (int i = 0; i < file_count; i++) {
            if (i == selected) attron(A_REVERSE);
            mvprintw(i + 2, 2, "%s", file_list[i]);
            if (i == selected) attroff(A_REVERSE);
        }
        refresh();

        int ch = getch();
        if (ch == KEY_UP && selected > 0) selected--;
        else if (ch == KEY_DOWN && selected < file_count - 1) selected++;
        else if (ch == 'q') break;
        else if (ch == 10) {
            download_selected_file(file_list[selected]);
            break;
        }
    }

    for (int i = 0; i < file_count; i++) free(file_list[i]);

    endwin();
    return 0;
}
