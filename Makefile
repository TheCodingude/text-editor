main: src/main.c
	gcc src/main.c -o main -lGL -lm -lSDL2 -ggdb -lSDL2_ttf -lssl -lcrypto
	./main 

old: old_rendering/old_rendering.c 
	gcc old_rendering/old_rendering.c -o old -lGL -lm -lSDL2 -ggdb 
	./old src/main.c

clean:
	git clean -fx


