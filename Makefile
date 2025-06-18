
main: src/main.c
	gcc src/main.c -o main -lGL -lm -lSDL2 -ggdb -lSDL2_ttf
	./main src/main.c

old: old_rendering/old_rendering.c 
	gcc old_rendering/old_rendering.c -o old -lGL -lm -lSDL2 -ggdb 
	./old

clean:
	git clean -fx