#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_ITEMS 2048

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

int is_directory(const char *dir, const char *name) {
    char fullpath[1024];
    struct stat st;

    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);
    if (stat(fullpath, &st) == -1) return 0;

    return S_ISDIR(st.st_mode);
}

int main() {
    strcpy(current_path, "/");

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    load_directory(current_path);

    while (1) {
        clear();
        mvprintw(0, 0, "Exploring: %s", current_path);

        for (int i = 0; i < item_count; i++) {
            if (i == selected)
                attron(A_REVERSE);

            if (is_directory(current_path, items[i]))
                mvprintw(i + 2, 2, "[%s]", items[i]);
            else
                mvprintw(i + 2, 2, "%s", items[i]);

            if (i == selected)
                attroff(A_REVERSE);
        }

        refresh();

        int ch = getch();

        if (ch == KEY_UP) {
            if (selected > 0) selected--;
        } 
        else if (ch == KEY_DOWN) {
            if (selected < item_count - 1) selected++;
        } 
        else if (ch == 10) {
	  if (is_directory(current_path, items[selected])) {
	    char new_path[1024];

	    if (strcmp(current_path, "/") == 0) {
	      strcpy(new_path, "/");
	      strncat(new_path, items[selected], sizeof(new_path) - strlen(new_path) - 1);
	    } else {
	      strcpy(new_path, current_path);
	      strncat(new_path, "/", sizeof(new_path) - strlen(new_path) - 1);
	      strncat(new_path, items[selected], sizeof(new_path) - strlen(new_path) - 1);
	    }

	    strcpy(current_path, new_path);
	    load_directory(current_path);
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
