#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define STRUNG_IMPLEMENTATION
#include "strung.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define FONT_8X16
#include "font.h"

typedef struct {
    int x_offset; // horizontal 
    int y_offset; // vertical 
} Scroll;

typedef struct {
    int pos_in_text;
    int pos_in_line;
    int line;
}Cursor;

typedef struct{
    SDL_Window *window;
    
    Scroll scroll;
    Cursor cursor;
    char* file_path;
    Strung text;
    bool selection;

    Cursor command_cursor;
    bool in_command;
    Strung command_text;
}Editor;

#include "filestuff.h"


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

void clamp_scroll(Editor *editor, Scroll *scroll, float scale) {
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);
    int lines_on_screen = h / (FONT_HEIGHT * scale);
    int cols_on_screen = w / (FONT_WIDTH * scale);

    // Count total lines
    int total_lines = 1;
    for (int i = 0; editor->text.data[i]; ++i) {
        if (editor->text.data[i] == '\n') total_lines++;
    }

    if (scroll->y_offset < 0) scroll->y_offset = 0;
    if (scroll->y_offset > total_lines - lines_on_screen)
        scroll->y_offset = total_lines - lines_on_screen;
    if (scroll->y_offset < 0) scroll->y_offset = 0;

    if (scroll->x_offset < 0) scroll->x_offset = 0;
    if (scroll->x_offset > 1000 - cols_on_screen) // max cols just because
        scroll->x_offset = 1000 - cols_on_screen;
    if (scroll->x_offset < 0) scroll->x_offset = 0;
}

void ensure_cursor_visible(Editor *editor, Scroll *scroll, float scale) {
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);
    int lines_on_screen = h / (FONT_HEIGHT * scale);
    int cols_on_screen = w / (FONT_WIDTH * scale);

    if (editor->cursor.line < scroll->y_offset)
        scroll->y_offset = editor->cursor.line;
    if (editor->cursor.line >= scroll->y_offset + lines_on_screen)
        scroll->y_offset = editor->cursor.line - lines_on_screen + 1;

    if (editor->cursor.pos_in_line < scroll->x_offset)
        scroll->x_offset = editor->cursor.pos_in_line;
    if (editor->cursor.pos_in_line >= scroll->x_offset + cols_on_screen)
        scroll->x_offset = editor->cursor.pos_in_line - cols_on_screen + 1;

    clamp_scroll(editor, scroll, scale);
}

void renderTextScrolled(char* text, float x, float y, float scale, Scroll *scroll) {
    int w, h;
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);
    int lines_on_screen = h / (FONT_HEIGHT * scale);
    int cols_on_screen = w / (FONT_WIDTH * scale);

    int line = 0, col = 0;
    float orig_x = x;
    float draw_y = y;
    float draw_x = x;
    for (int i = 0; text[i]; ++i) {
        if (line >= scroll->y_offset && line < scroll->y_offset + lines_on_screen) {
            if (col >= scroll->x_offset && col < scroll->x_offset + cols_on_screen) {
                if (text[i] != '\n') {
                    draw_char(text[i], draw_x + (col - scroll->x_offset) * FONT_WIDTH * scale, draw_y + (line - scroll->y_offset) * FONT_HEIGHT * scale, scale);
                }
            }
        }
        if (text[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
        if (line - scroll->y_offset > lines_on_screen) break; // safety
    }
}


void renderCursorScrolled(Editor *editor, float scale, Scroll *scroll) {
    int x = 10 + (editor->cursor.pos_in_line - scroll->x_offset) * FONT_WIDTH * scale;
    int y = 10 + (editor->cursor.line - scroll->y_offset) * FONT_HEIGHT * scale;
    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x, y + (FONT_HEIGHT * scale));
    glEnd();
}

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

void render_text_box(Editor *editor, char *buffer, char* prompt, float scale){
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);
    
    int prompt_width = strlen(prompt) * FONT_WIDTH * scale;
    
    float x = 0;
    float y = h;
    float x1 = x + w;
    float y1 = (90 * h) / 100;

    glColor4f(0.251, 0.251, 0.251, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(x, y1); // top left
    glVertex2f(x1, y1); // top right
    glVertex2f(x1, y); // bottom right
    glVertex2f(x, y); // bottom left
    glEnd();
    
    // rendering the cursor for the command box thingy
    int cx = x + 10 + prompt_width + editor->command_cursor.pos_in_text * FONT_WIDTH * scale;
    int cy = y - 40;
    
    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
    glVertex2f(cx, cy);
    glVertex2f(cx, cy + (FONT_HEIGHT * 2.0));
    glEnd();
    
    renderText(prompt, x + 10, y - 40, 2.0);
    renderText(editor->command_text.data, x + 10 + prompt_width, y - 40, 2.0);
}

int main(int argc, char *argv[]) {
    Editor editor = {.cursor = {0}, .file_path = "", .text = strung_init(""), .command_text = strung_init(""), .command_cursor = {0}};

    if (argc > 1){
        open_file(&editor, argv[1]);
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    editor.window = SDL_CreateWindow("Basic Text Editor",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!editor.window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(editor.window);
    if (!glContext) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(editor.window);
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
                if (!(SDL_GetModState() & KMOD_CTRL) && !editor.in_command) {
                    strung_insert_string(&editor.text, event.text.text, editor.cursor.pos_in_text);
                    editor.cursor.pos_in_text += strlen(event.text.text);
                    editor.cursor.pos_in_line += strlen(event.text.text);
                } else if(editor.in_command){
                    strung_insert_string(&editor.command_text, event.text.text, editor.command_cursor.pos_in_text);
                    editor.command_cursor.pos_in_text += strlen(event.text.text);
                }
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_BACKSPACE:
                        if (editor.in_command){
                            if(editor.command_cursor.pos_in_text > 0){
                                strung_remove_char(&editor.command_text, editor.command_cursor.pos_in_text - 1);
                                editor.command_cursor.pos_in_text--;
                            }
                        }
                        else if (editor.cursor.pos_in_text > 0) {
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
                    case SDLK_DELETE:
                        if (editor.in_command) {
                            if (editor.command_cursor.pos_in_text < editor.command_text.size) {
                                strung_remove_char(&editor.command_text, editor.command_cursor.pos_in_text);
                            }
                        } else if (editor.cursor.pos_in_text < editor.text.size) {
                            strung_remove_char(&editor.text, editor.cursor.pos_in_text);
                        }
                        break;
                    case SDLK_RETURN:
                        strung_insert_char(&editor.text, '\n', editor.cursor.pos_in_text);
                        editor.cursor.pos_in_text++;
                        editor.cursor.line++;
                        editor.cursor.pos_in_line = 0;
                        break;
                    case SDLK_TAB:
                        strung_insert_string(&editor.text, "    ", editor.cursor.pos_in_text);
                        editor.cursor.pos_in_line += 4;
                        editor.cursor.pos_in_text += 4;
                        break;
                    case SDLK_HOME: 
                        // Move cursor to the start of the current line
                        {
                            int pos = editor.cursor.pos_in_text;
                            while (pos > 0 && editor.text.data[pos - 1] != '\n') {
                                pos--;
                            }
                            editor.cursor.pos_in_text = pos;
                            editor.cursor.pos_in_line = 0;
                        }
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
                    case SDLK_F3:
                        strung_reset(&editor.command_text);
                        editor.command_cursor.pos_in_text = 0;
                        editor.in_command = !editor.in_command;
                        break;
                    case SDLK_LEFT:
                        if(editor.in_command){
                            if (editor.command_cursor.pos_in_text > 0) {
                                editor.command_cursor.pos_in_text--;
                            }
                        } else{
                            if (editor.cursor.pos_in_text > 0){
                                editor.cursor.pos_in_text--;
                            }
                            if(editor.cursor.pos_in_line > 0){
                                editor.cursor.pos_in_line--;
                            } else if(editor.cursor.line > 0){
                                editor.cursor.line--;
                            }
                        }
                        break;
                    case SDLK_RIGHT:
                        if(editor.in_command){
                            if (editor.command_cursor.pos_in_text < editor.command_text.size) {
                                editor.command_cursor.pos_in_text++;
                            }
                        }else{
                            if (editor.cursor.pos_in_text < editor.text.size){ 
                                editor.cursor.pos_in_text++;
                                editor.cursor.pos_in_line++;
                            }
                        }
                        break;
                    case SDLK_UP:
                        if(editor.cursor.line > 0){
                            editor_recalculate_pos_up(&editor);
                        }
                        ensure_cursor_visible(&editor, &editor.scroll, scale);
                        break;
                    case SDLK_DOWN:
                        editor_recalculate_pos_down(&editor);
                        ensure_cursor_visible(&editor, &editor.scroll, scale);
                        break;
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                    case SDLK_EQUALS:
                        if(event.key.keysym.mod & KMOD_CTRL){
                            if(event.key.keysym.mod & KMOD_ALT) scale = 2.0f;
                            else scale += 0.5;
                        }
                        break;
                    case SDLK_MINUS:
                        if(event.key.keysym.mod & KMOD_CTRL){  
                            if(event.key.keysym.mod & KMOD_ALT) scale = 2.0f;
                            else scale -= 0.5;
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
                    case SDLK_PAGEUP:
                        editor.scroll.y_offset -= 5;
                        editor.cursor.line -= 5;
                        clamp_scroll(&editor, &editor.scroll, scale);
                        break;
                    case SDLK_PAGEDOWN:
                        editor.scroll.y_offset += 5;
                        editor.cursor.line += 5;
                        clamp_scroll(&editor, &editor.scroll, scale);
                        break;
                    

                    default:
                        break;
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                editor.scroll.y_offset -= event.wheel.y;
                clamp_scroll(&editor, &editor.scroll, scale);
            }
        }

        // Ensure cursor is visible after any movement
        

        int w, h;
        SDL_GetWindowSize(editor.window, &w, &h);

        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        renderTextScrolled(editor.text.data, 10.0f, 10.0f, scale, &editor.scroll);

        if(editor.in_command){
            char buffer[100];
            render_text_box(&editor, buffer, "Enter Command: ", scale);
        }else{
            renderCursorScrolled(&editor, scale, &editor.scroll);
        }


        SDL_GL_SwapWindow(editor.window);
    }

    strung_free(&editor.text);
    SDL_StopTextInput();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(editor.window);
    SDL_Quit();

    return 0;
}
