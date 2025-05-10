#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

#define MAX_TEXT_LENGTH 1024

char text[MAX_TEXT_LENGTH] = "";
int text_length = 0;
int cursor_pos = 0;

void drawCursor(float x, float y, float scale, int pos) {
    char temp[MAX_TEXT_LENGTH];
    strncpy(temp, text, pos);
    temp[pos] = '\0';

    float cursor_x = stb_easy_font_width(temp) * scale;

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(x + cursor_x, y);
    glVertex2f(x + cursor_x, y + 20 * scale);
    glEnd();
}

void drawText(float x, float y, float scale, char* texts) {
    char buffer[99999];
    int num_quads;

    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    glColor3f(1.0f, 1.0f, 1.0f);

    num_quads = stb_easy_font_print(0, 0, texts, NULL, buffer, sizeof(buffer));

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Text Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_OPENGL);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 800, 600, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    SDL_StartTextInput();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_TEXTINPUT) {
                const char* input = event.text.text;
                while (*input && text_length < MAX_TEXT_LENGTH - 1) {
                    memmove(&text[cursor_pos + 1], &text[cursor_pos], text_length - cursor_pos + 1);
                    text[cursor_pos++] = *input++;
                    text_length++;
                }
                text[text_length] = '\0';
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) {
                    running = false;
                } else if (key == SDLK_BACKSPACE) {
                    if (cursor_pos > 0 && text_length > 0) {
                        memmove(&text[cursor_pos - 1], &text[cursor_pos], text_length - cursor_pos + 1);
                        cursor_pos--;
                        text_length--;
                    }
                } else if (key == SDLK_DELETE) {
                    if (cursor_pos < text_length) {
                        memmove(&text[cursor_pos], &text[cursor_pos + 1], text_length - cursor_pos);
                        text_length--;
                    }
                } else if (key == SDLK_RETURN || key == SDLK_RETURN2) {
                    if (text_length < MAX_TEXT_LENGTH - 1) {
                        memmove(&text[cursor_pos + 1], &text[cursor_pos], text_length - cursor_pos + 1);
                        text[cursor_pos++] = '\n';
                        text_length++;
                    }
                } else if (key == SDLK_LEFT) {
                    if (cursor_pos > 0) cursor_pos--;
                } else if (key == SDLK_RIGHT) {
                    if (cursor_pos < text_length) cursor_pos++;
                }
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        drawText(50.0f, 50.0f, 5.0f, text);
        drawCursor(50.0f, 50.0f, 5.0f, cursor_pos);

        SDL_GL_SwapWindow(window);
    }

    SDL_StopTextInput();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
