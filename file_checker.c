#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8081

char current_path[1024];

int main() {

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    //comm
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
      perror("socket creation failed");
      exit(-1);
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (connect(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
      perror("Connection failed");
      exit(-1);
    }

   
    //end comm
    int count = 0;

    clear();
    mvprintw(0, 0, "Your files on the server:");

    int idx = 0;

    while (1) {
      char c;
      int n = read(socket_fd, &c, 1);

      if (n <= 0)
	break;

      if (c == '\n') {
	current_path[idx] = '\0';
	idx = 0;

	if (strcmp(current_path, "END") == 0) {
	  break;
	}

	mvprintw(count + 2, 2, "%s", current_path);
	count++;

      } else {
	current_path[idx++] = c;
      }
    }

    refresh();
    close(socket_fd);
    
    while(1){
      int ch = getch();

      if (ch == 'q') {
	break;
      }
    }

   

    endwin();
    return 0;
}
