ncurses:
	sudo apt install libncurses5-dev libncursesw5-dev

all:
	gcc client.c -lncurses -o bin
	gcc file_loading.c -lncurses -o bin1
	gcc file_checker.c -lncurses -o bin2
	gcc file_download.c -lncurses -o bin3	
	gcc file_delete.c -lncurses -o bin4
	gcc folder_create.c -lncurses -o bin5
	./bin
