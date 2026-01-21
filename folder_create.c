#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_ITEMS 1024

#define CMD_GET_DIRS 5
#define CMD_MAKE_DIR 6

int global_sockfd;
char *remote_dirs[MAX_ITEMS];
int dir_count = 0;
int selected = 0;

void fetch_remote_dirs() {
    int cmd = CMD_GET_DIRS;
    write(global_sockfd, &cmd, sizeof(int));

    for (int i = 0; i < dir_count; i++) free(remote_dirs[i]);
    
    if (read(global_sockfd, &dir_count, sizeof(int)) <= 0) return;

    for (int i = 0; i < dir_count; i++) {
        int len;
        read(global_sockfd, &len, sizeof(int));
        remote_dirs[i] = malloc(len + 1);
        read(global_sockfd, remote_dirs[i], len);
        remote_dirs[i][len] = '\0';
    }
}

void get_folder_name(char *input) {
    echo();
    curs_set(TRUE);
    mvprintw(LINES - 2, 2, "Nume folder nou: ");
    clrtoeol();
    getnstr(input, 255);
    noecho();
    curs_set(FALSE);
}

void send_mkdir_request(const char *parent, const char *name) {
    int cmd = CMD_MAKE_DIR;
    write(global_sockfd, &cmd, sizeof(int));

    int p_len = strlen(parent);
    write(global_sockfd, &p_len, sizeof(int));
    write(global_sockfd, parent, p_len);

    int n_len = strlen(name);
    write(global_sockfd, &n_len, sizeof(int));
    write(global_sockfd, name, n_len);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Eroare: Lipseste socket-ul.\n");
        return 1;
    }
    global_sockfd = atoi(argv[1]);

    initscr();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(FALSE);

    fetch_remote_dirs();

    while (1) {
        clear();
        mvprintw(0, 2, "--- SERVER: Creare Director Nou ---");
        mvprintw(1, 2, "Selecteaza locatia unde vrei sa creezi folderul:");
        
        if (dir_count == 0) {
            mvprintw(3, 2, "(Niciun folder primit de pe server...)");
        }

        for (int i = 0; i < dir_count; i++) {
            if (i == selected) attron(A_REVERSE);
            mvprintw(i + 3, 4, "[DIR] %s", remote_dirs[i]);
            if (i == selected) attroff(A_REVERSE);
        }

        mvprintw(LINES - 1, 2, "ENTER: Selecteaza | R: Refresh | Q: Abandon");
        refresh();

        int ch = getch();
        if (ch == KEY_UP && selected > 0) selected--;
        else if (ch == KEY_DOWN && selected < dir_count - 1) selected++;
        else if (ch == 'r' || ch == 'R') {
            fetch_remote_dirs();
            selected = 0;
        }
        else if (ch == 10) {
            if (dir_count > 0) {
                char new_name[256];
                get_folder_name(new_name);
                
                if (strlen(new_name) > 0) {
                    send_mkdir_request(remote_dirs[selected], new_name);
                    mvprintw(LINES - 2, 2, "Folder creat cu succes!");
                    refresh();
                    sleep(1);
                    break;
                }
            }
        }
        else if (ch == 'q' || ch == 'Q') {
            break;
        }
    }

    for (int i = 0; i < dir_count; i++) free(remote_dirs[i]);
    endwin();
    return 0;
}
