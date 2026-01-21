#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_FILES 1024

int socket_fd;
char current_path[1024];

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Eroare: Lipseste descriptorul de socket.\n");
        exit(1);
    }
    
    socket_fd = atoi(argv[1]);
    
    int op = 2;
    write(socket_fd, &op, sizeof(int));

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
   
    int count = 0;
    clear();
    mvprintw(0, 0, "Fisierele tale de pe server:");
    mvprintw(1, 0, "(Apasati 'q' pentru a iesi)");
    refresh();

    int idx = 0;
    while (count < MAX_FILES) {
        char c;
        int n = read(socket_fd, &c, 1);

        if (n <= 0) break;

        if (c == '\n') {
            current_path[idx] = '\0';
            idx = 0;

            if (strcmp(current_path, "END") == 0) {
                break;
            }

            mvprintw(count + 3, 2, "%s", current_path);
            count++;
            refresh();

        } else {
            if (idx < (int)sizeof(current_path) - 1) {
                current_path[idx++] = c;
            }
        }
    }

    while(1){
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
    }

    endwin();
    return 0;
}
