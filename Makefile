
main: main.c
	gcc main.c -o main -lGL -lm -lSDL2 -ggdb
	./main main.c

test: test.c
	gcc test.c -o test -lGL -lm -lSDL2 -ggdb
	./test

clean:
	git clean -fx