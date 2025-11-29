#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_ITEMS 2048
#define PORT 8081

char current_path[1024];
char *items[MAX_ITEMS];
int item_count = 0;
int selected = 0;

void load_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;

    for (int i = 0; i < item_count; i++)
        free(items[i]);
    item_count = 0;
    selected = 0;

    dir = opendir(path);
    if (!dir) return;

    while ((entry = readdir(dir)) != NULL) {
        items[item_count++] = strdup(entry->d_name);
        if (item_count >= MAX_ITEMS) break;
    }

    closedir(dir);
}

int main() {
    strcpy(current_path, "/");

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
    
    while (1) {  
      int n;
      if((n = read(socket_fd,current_path, sizeof(current_path)-1)) > 0){
	break;
      }
      current_path[n] = '\0';

      mvprintw(count + 2, 2, "%s", current_path);
      count++;
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
