#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 256

char text[MAX_LINES][MAX_LINE_LENGTH] = {{0}};
int num_lines = 1;
int cursor_x = 0, cursor_y = 0;

SDL_Window *window = NULL;
SDL_GLContext gl_context;
TTF_Font *font = NULL;

void drawText(int x, int y, const char *str, SDL_Color color) {
    if (!str || str[0] == '\0') return;
    SDL_Surface *surface = TTF_RenderText_Blended(font, str, color);
    if (!surface) return;

    // Convert surface to RGBA8888 format for OpenGL compatibility
    SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!converted) return;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload the converted surface as a texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1, 1, 1, 1);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2i(x, y);
    glTexCoord2f(1, 0); glVertex2i(x + converted->w, y);
    glTexCoord2f(1, 1); glVertex2i(x + converted->w, y + converted->h);
    glTexCoord2f(0, 1); glVertex2i(x, y + converted->h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDeleteTextures(1, &tex);
    SDL_FreeSurface(converted);
}

void render() {
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    SDL_Color white = {255, 255, 255, 255};
    int line_height = TTF_FontLineSkip(font) + 2;
    for (int i = 0; i < num_lines; ++i) {
        drawText(10, 10 + i * line_height, text[i], white);
    }

    // Draw cursor as a red rectangle
    int cursor_px = 10;
    if (cursor_x > 0) {
        char buf[MAX_LINE_LENGTH];
        int line_len = strlen(text[cursor_y]);
        int safe_cursor_x = cursor_x > line_len ? line_len : cursor_x;
        strncpy(buf, text[cursor_y], safe_cursor_x);
        buf[safe_cursor_x] = 0;
        int w = 0;
        TTF_SizeText(font, buf, &w, NULL);
        cursor_px += w;
    }
    int cursor_py = 10 + cursor_y * line_height;

    glColor3f(1, 0, 0);
    glBegin(GL_LINES);
    glVertex2i(cursor_px, cursor_py);
    glVertex2i(cursor_px, cursor_py + line_height);
    glEnd();

    SDL_GL_SwapWindow(window);
}

int main(int argc, char **argv) {
    memset(text, 0, sizeof(text));
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    window = SDL_CreateWindow("SDL2 Text Editor (ESC to quit)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
    gl_context = SDL_GL_CreateContext(window);
    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 20);
    if (!font) {
        printf("Font not found! SDL_ttf error: %s\n", TTF_GetError());
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    int running = 1;
    SDL_StartTextInput();
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode key = e.key.keysym.sym;
                if (key == SDLK_ESCAPE) running = 0;
                else if (key == SDLK_RETURN) {
                    if (num_lines < MAX_LINES - 1) {
                        for (int i = num_lines; i > cursor_y + 1; --i)
                            strcpy(text[i], text[i - 1]);
                        strcpy(text[cursor_y + 1], text[cursor_y] + cursor_x);
                        text[cursor_y][cursor_x] = '\0';
                        cursor_y++;
                        cursor_x = 0;
                        num_lines++;
                    }
                } else if (key == SDLK_BACKSPACE) {
                    if (cursor_x > 0) {
                        memmove(&text[cursor_y][cursor_x - 1], &text[cursor_y][cursor_x], strlen(&text[cursor_y][cursor_x]) + 1);
                        cursor_x--;
                    } else if (cursor_y > 0) {
                        int prev_len = strlen(text[cursor_y - 1]);
                        if (prev_len + strlen(text[cursor_y]) < MAX_LINE_LENGTH) {
                            strcat(text[cursor_y - 1], text[cursor_y]);
                            for (int i = cursor_y; i < num_lines - 1; ++i)
                                strcpy(text[i], text[i + 1]);
                            num_lines--;
                            cursor_y--;
                            cursor_x = prev_len;
                        }
                    }
                } else if (key == SDLK_LEFT) {
                    if (cursor_x > 0) cursor_x--;
                    else if (cursor_y > 0) {
                        cursor_y--;
                        cursor_x = strlen(text[cursor_y]);
                    }
                } else if (key == SDLK_RIGHT) {
                    if (cursor_x < strlen(text[cursor_y])) cursor_x++;
                    else if (cursor_y < num_lines - 1) {
                        cursor_y++;
                        cursor_x = 0;
                    }
                } else if (key == SDLK_UP) {
                    if (cursor_y > 0) {
                        cursor_y--;
                        if (cursor_x > strlen(text[cursor_y]))
                            cursor_x = strlen(text[cursor_y]);
                    }
                } else if (key == SDLK_DOWN) {
                    if (cursor_y < num_lines - 1) {
                        cursor_y++;
                        if (cursor_x > strlen(text[cursor_y]))
                            cursor_x = strlen(text[cursor_y]);
                    }
                }
            } else if (e.type == SDL_TEXTINPUT) {
                // Only allow printable ASCII for now
                const char *input = e.text.text;
                size_t input_len = strlen(input);
                int len = strlen(text[cursor_y]);
                // Only insert if all bytes are printable ASCII and fits in line
                int valid = 1;
                for (size_t i = 0; i < input_len; ++i) {
                    if (input[i] < 32 || input[i] > 126) {
                        valid = 0;
                        break;
                    }
                }
                if (valid && len + input_len < MAX_LINE_LENGTH) {
                    memmove(&text[cursor_y][cursor_x + input_len], &text[cursor_y][cursor_x], len - cursor_x + 1);
                    memcpy(&text[cursor_y][cursor_x], input, input_len);
                    cursor_x += input_len;
                }
            }
        }
        render();
        SDL_Delay(10);
    }
    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}