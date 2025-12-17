#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8081
#define MAX_FILES 1024
#define MAX_PATH 1024

char files[MAX_FILES][MAX_PATH];
int file_count = 0;

int main(void) {
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        endwin();
        perror("socket");
        exit(1);
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (connect(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        endwin();
        perror("connect");
        exit(1);
    }

    char buffer[MAX_PATH];
    int idx = 0;

    while (1) {
        char c;
        int n = read(socket_fd, &c, 1);
        if (n <= 0) break;

        if (c == '\n') {
            buffer[idx] = '\0';
            idx = 0;

            if (strcmp(buffer, "END") == 0)
                break;

            strncpy(files[file_count++], buffer, MAX_PATH);
        } else {
            if (idx < MAX_PATH - 1)
                buffer[idx++] = c;
        }
    }

    int selected = 0;
    int offset = 0;
    int max_y, max_x;

    while (1) {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        mvprintw(0, 0, "Server files (ENTER = delete, q = quit)");

        int visible = max_y - 2;

        for (int i = 0; i < visible && i + offset < file_count; i++) {
            int idx = i + offset;

            if (idx == selected)
                attron(A_REVERSE);

            mvprintw(i + 1, 2, "%s", files[idx]);

            if (idx == selected)
                attroff(A_REVERSE);
        }

        refresh();
        int ch = getch();

        if (ch == 'q')
            break;

        if (ch == KEY_UP) {
            if (selected > 0)
                selected--;
            if (selected < offset)
                offset--;
        }

        if (ch == KEY_DOWN) {
            if (selected < file_count - 1)
                selected++;
            if (selected >= offset + visible)
                offset++;
        }

        if (ch == '\n') {
            write(socket_fd, files[selected], strlen(files[selected]));
            write(socket_fd, "\n", 1);

            break;
        }
    }

    close(socket_fd);
    endwin();
    return 0;
}
