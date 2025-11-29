#include <ncurses.h>
#include <string.h>

typedef struct {
  WINDOW *win;
  int x, y, w, h;
  const char *label;
} Button;

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

int main() {
  initscr();
  noecho();
  curs_set(FALSE);
  keypad(stdscr, TRUE);
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

  int h, w;
  getmaxyx(stdscr, h, w);

  int bh = 3, bw = 15;
  int y = h - bh - 1;

  Button b1, b2, b3;

  // Setarea butoanelor
  b1 = (Button){ newwin(bh, bw, y, w/6 - bw/2), w/6 - bw/2, y, bw, bh, "Buton 1" };
  b2 = (Button){ newwin(bh, bw, y, w/2 - bw/2), w/2 - bw/2, y, bw, bh, "Buton 2" };
  b3 = (Button){ newwin(bh, bw, y, 5*w/6 - bw/2), 5*w/6 - bw/2, y, bw, bh, "Buton 3" };
  refresh();

  draw_button(&b1, 0);
  draw_button(&b2, 0);
  draw_button(&b3, 0);
  refresh();

  MEVENT mevent;
  int ch;

  while ((ch = getch()) != 'q') {

    if (ch == KEY_MOUSE) {
      if (getmouse(&mevent) == OK) {

	int mx = mevent.x;
	int my = mevent.y;

 	if (mevent.bstate & BUTTON1_CLICKED) {

	  if (click_inside(&b1, mx, my)) {
	    draw_button(&b1, 1);
	    refresh();
	    mvprintw(0, 0, "Ai dat click pe Buton 1   ");
	    napms(100);
	    draw_button(&b1, 0);
	  } 
	  else if (click_inside(&b2, mx, my)) {
	    draw_button(&b2, 1);
	    refresh();
	    mvprintw(0, 0, "Ai dat click pe Buton 2   ");
	    napms(100);
	    draw_button(&b2, 0);
	  } 
	  else if (click_inside(&b3, mx, my)) {
	    draw_button(&b3, 1);
	    refresh();
	    mvprintw(0, 0, "Ai dat click pe Buton 3   ");
	    napms(100);
	    draw_button(&b3, 0);
	  }

	  refresh();
	}
      }
    }
  }

  delwin(b1.win);
  delwin(b2.win);
  delwin(b3.win);
  endwin();
  return 0;
}
