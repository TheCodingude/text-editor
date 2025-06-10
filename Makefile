
main: main.c
	gcc main.c -o main -lGL -lm -lSDL2 -ggdb
	./main main.c

clean:
	git clean -fx