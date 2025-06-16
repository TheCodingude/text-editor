
main: main.c
	gcc main.c -o main -lGL -lm -lSDL2 -ggdb -lSDL2_ttf
	./main main.c

test: Editor_ttf_test.c
	gcc Editor_ttf_test.c -o test -lGL -lm -lSDL2 -ggdb -lSDL2_ttf
	./test

clean:
	git clean -fx