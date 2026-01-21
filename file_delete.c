#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_FILES 1024

char *files[MAX_FILES];
int file_count = 0;
int global_sockfd;

int main(int argc, char **argv) {
  if (argc < 2) {
    return 1; 
  }

  global_sockfd = atoi(argv[1]);
  int op = 4;
  write(global_sockfd,&op,sizeof(int));

  initscr();
  noecho();
  curs_set(FALSE);
  keypad(stdscr, TRUE);

  char buffer[1024];
  int idx = 0;

  while (file_count < MAX_FILES) {
    char c;
    int n = read(global_sockfd, &c, 1);
    if (n <= 0) break;

    if (c == '\n') {
      buffer[idx] = '\0';
      idx = 0;

      if (strcmp(buffer, "END") == 0)
	break;

      files[file_count++] = strdup(buffer);
    } else {
      buffer[idx++] = c;
    }
  }

  int selected = 0;

  while (1) {
    clear();

    mvprintw(0, 0, "DELETE MODE: Select file and press ENTER ('q' to cancel)");

    for (int i = 0; i < file_count; i++) {
      if (i == selected)attron(A_REVERSE);
      mvprintw(i + 2, 2, "%s", files[i]);
      if (i == selected)attroff(A_REVERSE);
    }

    refresh();
    int ch = getch();

    if (ch == 'q'){
      write(global_sockfd, "\n", 1);
      break;
    }

    if (ch == KEY_UP && selected > 0) selected--;

    if (ch == KEY_DOWN && selected < file_count - 1)selected++;

    if (ch == '\n') {
      write(global_sockfd, files[selected], strlen(files[selected]));
      write(global_sockfd, "\n", 1);
      refresh();
      napms(500); 
      break;
    }
  }

  for (int i = 0; i < file_count; i++) free(files[i]);

  endwin();
  return 0;
}
