#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 4555
#define SERVER_IP "127.0.0.1"
#define OP_UPLOAD 1
#define OP_CHECK 2
#define OP_DOWNLOAD 3
#define OP_DELETE 4

typedef struct {
  WINDOW *win;
  int x, y, w, h;
  const char *label;
} Button;

int global_sockfd = -1;
char username[50];

void draw_button(Button *btn, int highlighted) {
  if (highlighted) wattron(btn->win, A_REVERSE);
  box(btn->win, 0, 0);
  mvwprintw(btn->win, 1, (btn->w - strlen(btn->label)) / 2, "%s", btn->label);
  if (highlighted) wattroff(btn->win, A_REVERSE);
  wrefresh(btn->win);
}

int click_inside(Button *btn, int mx, int my) {
  return (mx >= btn->x && mx < btn->x + btn->w &&
          my >= btn->y && my < btn->y + btn->h);
}

void connect_and_identify(int op) {
    struct sockaddr_in server_addr;
    global_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (global_sockfd < 0) {
        endwin();
        perror("Eroare creare socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(global_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        endwin();
        printf("Serverul nu raspunde la %s:%d\n", SERVER_IP, SERVER_PORT);
        exit(1);
    }

    int name_len = strlen(username);
    write(global_sockfd, &name_len, sizeof(int));
    write(global_sockfd, username, name_len);
    write(global_sockfd,&op,sizeof(int));
}

int main() {
  initscr();
  noecho();
  curs_set(FALSE);
  keypad(stdscr, TRUE);
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

  int h, w;
  getmaxyx(stdscr, h, w);

  int len = 0;
  int ch;

  while (1) {
    clear();
    mvprintw(3, w/2-sizeof("Enter your name:"), "Login");
    mvprintw(5, w/2-sizeof("Enter your name:"), "Enter your name:");
    mvprintw(6, w/2-sizeof("Enter your name:"), "> %s", username);
    mvprintw(8, w/2-sizeof("Enter your name:"), "Press ENTER to continue");

    move(6, 7 + len);
    refresh();

    ch = getch();

    if(ch == 27){
      endwin();
      exit(0);
    }

    if (ch == '\n') {
      if (len > 0) {
	break;
      } else {
	mvprintw(10, w/2-sizeof("Enter your name:"), "Name cannot be empty!");
	refresh();
	napms(1000);
      }
    }
    else if (ch == KEY_BACKSPACE || ch == 127) {
      if (len > 0) {
	username[--len] = '\0';
      }
    }
    else if (ch >= 32 && ch <= 126 && len < (int)sizeof(username) - 1) {
      username[len++] = ch;
      username[len] = '\0';
    }
  }

  clear();
  refresh();

  //connect_and_identify();

  int bh = 3, bw = 15;
  int y = h - bh - 1;

  int col = w / 4;

  Button b1, b2, b3, b4;

  b1 = (Button){ newwin(bh, bw, y, col/2 - bw/2),     col/2 - bw/2,     y, bw, bh, "Upload" };
  b2 = (Button){ newwin(bh, bw, y, col + col/2 - bw/2), col + col/2 - bw/2, y, bw, bh, "Check" };
  b3 = (Button){ newwin(bh, bw, y, 2*col + col/2 - bw/2), 2*col + col/2 - bw/2, y, bw, bh, "Download" };
  b4 = (Button){ newwin(bh, bw, y, 3*col + col/2 - bw/2), 3*col + col/2 - bw/2, y, bw, bh, "Delete" };

  refresh();

  draw_button(&b1, 0);
  draw_button(&b2, 0);
  draw_button(&b3, 0);
  draw_button(&b4, 0);

  MEVENT mevent;

  while ((ch = getch()) != 'q') {
    if (ch == KEY_MOUSE && getmouse(&mevent) == OK) {

      int mx = mevent.x;
      int my = mevent.y;

      if (mevent.bstate & BUTTON1_CLICKED) {
        int op=OP_UPLOAD;
        connect_and_identify(op);
        if (click_inside(&b1, mx, my)) {
          draw_button(&b1, 1);
          napms(100);
          draw_button(&b1, 0);

          pid_t copil = fork();
          if (copil == 0) {
            char sock_str[10];
	    sprintf(sock_str, "%d", global_sockfd);              
	    execl("./bin1", "./bin1", sock_str, NULL);
            perror("execl failed");
            exit(1);
          } else {
            wait(NULL);
            endwin();
            initscr();
            noecho();
            curs_set(FALSE);
            keypad(stdscr, TRUE);
            mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
          }
        }

        else if (click_inside(&b2, mx, my)) {
          int op=OP_CHECK;
          connect_and_identify(op);
          draw_button(&b2, 1);
          napms(100);
          draw_button(&b2, 0);

          pid_t copil = fork();
          if (copil == 0) {
            char sock_str[10];
	    sprintf(sock_str, "%d", global_sockfd);              
	    execl("./bin2", "./bin2", sock_str, NULL);
            perror("execl failed");
            exit(1);
          } else {
            wait(NULL);
            endwin();
            initscr();
            noecho();
            curs_set(FALSE);
            keypad(stdscr, TRUE);
            mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
          }
        }

        else if (click_inside(&b3, mx, my)) {
          int op=OP_DOWNLOAD;
          connect_and_identify(op);
	  draw_button(&b3, 1);
          napms(100);
          draw_button(&b3, 0);

          pid_t copil = fork();
          if (copil == 0) {
            char sock_str[10];
	    sprintf(sock_str, "%d", global_sockfd);              
	    execl("./bin3", "./bin3", sock_str, NULL);
            perror("execl failed");
            exit(1);
          } else {
            wait(NULL);
            endwin();
            initscr();
            noecho();
            curs_set(FALSE);
            keypad(stdscr, TRUE);
            mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
          }
        }

        else if (click_inside(&b4, mx, my)) {
          int op=OP_DELETE;
          connect_and_identify(op);
	  draw_button(&b4, 1);
          napms(100);
          draw_button(&b4, 0);

          pid_t copil = fork();
          if (copil == 0) {
            char sock_str[10];
	    sprintf(sock_str, "%d", global_sockfd);              
	    execl("./bin4", "./bin4", sock_str, NULL);
            perror("execl failed");
            exit(1);
          } else {
            wait(NULL);
            endwin();
            initscr();
            noecho();
            curs_set(FALSE);
            keypad(stdscr, TRUE);
            mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
          }
        }

        draw_button(&b1, 0);
        draw_button(&b2, 0);
        draw_button(&b3, 0);
        draw_button(&b4, 0);
        refresh();
      }
    }
  }

  delwin(b1.win);
  delwin(b2.win);
  delwin(b3.win);
  delwin(b4.win);
  endwin();
  return 0;
}