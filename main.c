#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string.h>

#include "stb_easy_font.h" 

#define STRUNG_IMPLEMENTATION
#include "strung.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct {
    int pos;
    int line;
}Cursor;


void renderText(char* text, float x, float y, float scale) {
    char buffer[99999]; // Temporary buffer for stb_easy_font
    int num_quads;

    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Generate quads for the text
    num_quads = stb_easy_font_print(x, y, text, NULL, buffer, sizeof(buffer));

    // Render the quads
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Basic Text Editor",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_OPENGL);
    if (!window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_StartTextInput();

    Strung text = strung_init("");
    Cursor cursor = {0};
    float scale = 3.0f;

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_TEXTINPUT) {
                if (!(SDL_GetModState() & KMOD_CTRL)) {
                    strung_insert_string(&text, event.text.text, cursor.pos);
                    cursor.pos += strlen(event.text.text);
                    
                } else {}
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_BACKSPACE:
                        if (cursor.pos > 0) {
                            strung_remove_char(&text, cursor.pos-1);
                            cursor.pos--;
                        }
                        break;
                    case SDLK_RETURN:
                        strung_insert_char(&text, '\n', cursor.pos);
                        cursor.pos++;
                        break;
                    case SDLK_LEFT:
                        if (cursor.pos > 0) cursor.pos--;
                        break;
                    case SDLK_RIGHT:
                        if (cursor.pos < text.size) cursor.pos++;
                        break;
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                    case SDLK_EQUALS:
                        if(event.key.keysym.mod & KMOD_CTRL){
                            scale += 0.5;
                        }
                        break;
                    case SDLK_MINUS:
                        if(event.key.keysym.mod & KMOD_CTRL){  
                            if(scale > 0.5)
                                scale -= 0.5;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        renderText(text.data, 10.0f, 10.0f, scale);
        // printf("%f \n", scale);


        SDL_GL_SwapWindow(window);
    }

    strung_free(&text);
    SDL_StopTextInput();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
