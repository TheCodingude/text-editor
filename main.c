#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string.h>
#include <assert.h>


#define STRUNG_IMPLEMENTATION
#include "strung.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct {
    int pos_in_text;
    int pos_in_line;
    int line;
}Cursor;

typedef struct{
    Cursor cursor;
    char* file_path;
    Strung text;
}Editor;

#define FONT_8X16
#include "font.h"
#include "filestuff.h"


void editor_recalculate_pos_up(Editor* editor){
    int target_line = editor->cursor.line - 1;
    int col = editor->cursor.pos_in_line;
    int pos = 0;
    int current_line = 0;
    int current_col = 0;

    while (editor->text.data[pos] && current_line < target_line) {
        if (editor->text.data[pos] == '\n') {
            current_line++;
        }
        pos++;
    }

    int line_start = pos;

    while (editor->text.data[pos] && editor->text.data[pos] != '\n' && current_col < col) {
        pos++;
        current_col++;
    }
    editor->cursor.pos_in_text = pos;
    editor->cursor.pos_in_line = current_col;
    if(editor->cursor.line > 0){
        editor->cursor.line--;
    }
}

void editor_recalculate_pos_down(Editor* editor){
    int target_line = editor->cursor.line + 1;
    int col = editor->cursor.pos_in_line;
    int pos = 0;
    int current_line = 0;
    int current_col = 0;

    // Find start of target line
    while (editor->text.data[pos] && current_line < target_line) {
        if (editor->text.data[pos] == '\n') {
            current_line++;
        }
        pos++;
    }


    if (!editor->text.data[pos]) return;

    int line_start = pos;

    while (editor->text.data[pos] && editor->text.data[pos] != '\n' && current_col < col) {
        pos++;
        current_col++;
    }
    editor->cursor.pos_in_text = pos;
    editor->cursor.pos_in_line = current_col;
    editor->cursor.line++;
}

void draw_char(char c, float x, float y, float scale) {
    if (c < 32 || c > 126) return; // Only printable ASCII
    int idx = c - 32;
    for (int row = 0; row < FONT_HEIGHT; ++row) {
        unsigned char bits = font8x16[idx][row];
        for (int col = 0; col < FONT_WIDTH; ++col) {
            if (bits & (1 << (7 - col))) {
                float px = x + col * scale;
                float py = y + row * scale;
                glBegin(GL_QUADS);
                glVertex2f(px, py);
                glVertex2f(px + scale, py);
                glVertex2f(px + scale, py + scale);
                glVertex2f(px, py + scale);
                glEnd();
            }
        }
    }
}

// Render a string using the bitmap font
void renderText(char* text, float x, float y, float scale) {
    glColor3f(1.0f, 1.0f, 1.0f);
    float orig_x = x;
    for (int i = 0; text[i]; ++i) {
        if (text[i] == '\n') {
            y += FONT_HEIGHT * scale;
            x = orig_x;
        } else {
            draw_char(text[i], x, y, scale);
            x += FONT_WIDTH * scale;
        }
    }
}

void renderCursor(Editor *editor, float scale) {
    int x = 0, y = 0;
    // Calculate cursor position in (column, row)
    // int col = 0, row = 0;
    // for (int i = 0; i < editor->cursor.pos_in_text; ++i) {
    //     if (editor->text.data[i] == '\n') {
    //         row++;
    //         col = 0;
    //     } else {
    //         col++;
    //     }
    // }
    x = 10 + editor->cursor.pos_in_line * FONT_WIDTH * scale;
    y = 10 + editor->cursor.line * FONT_HEIGHT * scale;
    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x, y + (FONT_HEIGHT * scale));
    glEnd();
}

int main(int argc, char *argv[]) {
    Editor editor = {.cursor = {0}, .file_path = "", .text = strung_init("")};

    if (argc > 1){
        open_file(&editor, argv[1]);
    }


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

    float scale = 2.0f;

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_TEXTINPUT) {
                if (!(SDL_GetModState() & KMOD_CTRL)) {
                    strung_insert_string(&editor.text, event.text.text, editor.cursor.pos_in_text);
                    editor.cursor.pos_in_text += strlen(event.text.text);
                    editor.cursor.pos_in_line += strlen(event.text.text);
                    
                } else {}
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_BACKSPACE:
                        if (editor.cursor.pos_in_text > 0) {
                            // Check if deleting a newline
                            if (editor.text.data[editor.cursor.pos_in_text - 1] == '\n') {
                                // Move cursor to end of previous line
                                int pos = editor.cursor.pos_in_text - 2;
                                int col = 0;
                                while (pos >= 0 && editor.text.data[pos] != '\n') {
                                    pos--;
                                    col++;
                                }
                                editor.cursor.pos_in_text--;
                                editor.cursor.line--;
                                editor.cursor.pos_in_line = col;
                                strung_remove_char(&editor.text, editor.cursor.pos_in_text);
                            } else {
                                strung_remove_char(&editor.text, editor.cursor.pos_in_text - 1);
                                editor.cursor.pos_in_text--;
                                if (editor.cursor.pos_in_line > 0) editor.cursor.pos_in_line--;
                            }
                        }
                        break;
                    case SDLK_RETURN:
                        strung_insert_char(&editor.text, '\n', editor.cursor.pos_in_text);
                        editor.cursor.pos_in_text++;
                        editor.cursor.line++;
                        editor.cursor.pos_in_line = 0;
                        break;
                    case SDLK_HOME: 
                        // Move cursor to the start of the current line
                        int pos = editor.cursor.pos_in_text;
                        while (pos > 0 && editor.text.data[pos - 1] != '\n') {
                            pos--;
                        }
                        editor.cursor.pos_in_text = pos;
                        editor.cursor.pos_in_line = 0;
                        break;
                    case SDLK_END:
                        {
                            int pos = editor.cursor.pos_in_text;
                            int col = editor.cursor.pos_in_line;
                            // Move to end of line or end of text
                            while (editor.text.data[pos] && editor.text.data[pos] != '\n') {
                                pos++;
                                col++;
                            }
                            editor.cursor.pos_in_text = pos;
                            editor.cursor.pos_in_line = col;
                        }
                        break;
                    case SDLK_LEFT:
                        if (editor.cursor.pos_in_text > 0){
                            editor.cursor.pos_in_text--;
                            editor.cursor.pos_in_line--;
                        } 
                        break;
                    case SDLK_RIGHT:
                        if (editor.cursor.pos_in_text < editor.text.size){ 
                            editor.cursor.pos_in_text++;
                            editor.cursor.pos_in_line++;
                        }
                        break;
                    case SDLK_UP:
                        editor_recalculate_pos_up(&editor);
                        break;
                    case SDLK_DOWN:
                        editor_recalculate_pos_down(&editor);
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
                    case SDLK_s:
                        if(event.key.keysym.mod & KMOD_CTRL){
                            save_file(&editor);
                        }
                        break;
                    case SDLK_o:
                        if(event.key.keysym.mod & KMOD_CTRL){
                            // open_file();
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

        renderText(editor.text.data, 10.0f, 10.0f, scale);
        renderCursor(&editor, scale);


        SDL_GL_SwapWindow(window);
    }

    strung_free(&editor.text);
    SDL_StopTextInput();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
