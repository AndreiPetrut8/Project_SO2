ncurses:
	sudo apt install libncurses5-dev libncursesw5-dev

all:
	gcc client.c -lncurses -o bin
	gcc file_loading.c -lncurses -o bin1
	./bin
