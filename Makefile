CFLAGS = $(shell pkg-config --cflags freetype2 sdl2)
LDFLAGS = $(shell pkg-config --libs freetype2 sdl2) -lGL

<<<<<<< HEAD
main: src/main.c
	gcc src/main.c -o main -lGL -lm -lSDL2 -ggdb -lSDL2_ttf
	./main src/main.c

old: old_rendering/old_rendering.c 
	gcc old_rendering/old_rendering.c -o old -lGL -lm -lSDL2 -ggdb 
	./old
=======
main: main.c
	gcc $(CFLAGS) main.c -o main $(LDFLAGS)
	./main main.c
>>>>>>> afe05f9 (updated things)

test: test.c
	gcc $(CFLAGS) test.c -o test $(LDFLAGS) -lGLEW
	./test test.c

clean:
	git clean -fx


