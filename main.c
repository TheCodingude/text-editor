#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifndef _WIN32
#include <dirent.h>
#else
#error "We currently do not support windows"
#endif // _WIN32

#define STRUNG_IMPLEMENTATION
#include "strung.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// selection swap
#define SEL_SWAP(a, b) do{\
    int tmp = a;          \
    a = b;                \
    b = tmp;              \
}while(0);

 


#define FONT_8X16
#include "font.h"

typedef struct{
    char* name;
    int type; // Just using d_type
    struct stat data;
}FB_item;

typedef struct{
    FB_item *items;
    int count;
    int capacity;
}FB_items;

static void FB_items_append(FB_items *list, FB_item item) {
    if (list->count == list->capacity) {
        int new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        FB_item *new_items = (FB_item*)realloc(list->items, new_capacity * sizeof(FB_item));
        if (!new_items) {
            fprintf(stderr, "Failed to allocate memory in %s\n", __func__);
            exit(1);
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = item;
}


typedef struct{
    bool file_browser;
    size_t cursor;
    float scale;
    
    Strung new_file_path;
    bool new_file;          // surely there is a better way to do this right?
    bool new_dir;
    bool renaming;

    Strung relative_path;

    FB_items items;
}File_Browser;

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
    size_t start;
    size_t end;
}Line;

typedef struct{
    Line *lines;
    size_t size;
    size_t capacity;
}Lines;


typedef struct{
    SDL_Window *window;
    

    Scroll scroll;
    Cursor cursor;
    char* file_path;
    
    Strung text;
    Lines lines;

    int selection_start;
    int selection_end;
    bool selection;

    Cursor command_cursor;
    bool in_command;
    Strung command_text;
}Editor;

void editor_recalculate_lines(Editor *editor);

#include "filestuff.h"
#include "la.c"

#define WHITE vec4f(1.0f, 1.0f, 1.0f, 1.0f)
#define LINE_NUMS_OFFSET (FONT_WIDTH * scale) * 5.0f + 10.0f

void render_selection(Editor *editor, float scale, Scroll *scroll) {
    if (!editor->selection || editor->selection_start == editor->selection_end) {
        return;
    }

    int sel_start = editor->selection_start;
    int sel_end = editor->selection_end;
    if (sel_start > sel_end) {
        int tmp = sel_start;
        sel_start = sel_end;
        sel_end = tmp;
    }

    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);
    int lines_on_screen = h / (FONT_HEIGHT * scale);
    int cols_on_screen = w / (FONT_WIDTH * scale);

    // Offset for line nums
    float line_number_offset = LINE_NUMS_OFFSET;

    int line = 0, col = 0;
    int i = 0;
    while (editor->text.data[i]) {
        if (i >= sel_start && i < sel_end && editor->text.data[i] != '\n') {
            int draw_line = line - scroll->y_offset;
            int draw_col = col - scroll->x_offset;
            if (draw_line >= 0 && draw_line < lines_on_screen &&
                draw_col >= 0 && draw_col < cols_on_screen) {

                float x = line_number_offset + draw_col * FONT_WIDTH * scale;
                float y = 10 + draw_line * FONT_HEIGHT * scale;
                glColor4f(0.2f, 0.5f, 1.0f, 0.3f); // semi-transparent blue
                glBegin(GL_QUADS);
                glVertex2f(x, y);
                glVertex2f(x + FONT_WIDTH * scale, y);
                glVertex2f(x + FONT_WIDTH * scale, y + FONT_HEIGHT * scale);
                glVertex2f(x, y + FONT_HEIGHT * scale);
                glEnd();
            }
        }
        if (editor->text.data[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
        i++;
        if (i >= sel_end) break;
    }
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // reset color
}


void editor_move_cursor_to_click(Editor* editor, int x, int y, float scale) {
    int line_height = FONT_HEIGHT * scale;
    int char_width = FONT_WIDTH * scale;

    // Adjust for padding and scroll
    int relative_y = y - 10;
    int relative_x = x - (LINE_NUMS_OFFSET); // account for line numbas

    int clicked_line = (relative_y / line_height) + editor->scroll.y_offset;
    int clicked_col  = relative_x / char_width;

    // Clamp line to available lines
    if (clicked_line < 0) clicked_line = 0;
    if (clicked_line >= (int)editor->lines.size) clicked_line = editor->lines.size - 1;

    Line line = editor->lines.lines[clicked_line];
    int line_length = line.end - line.start;

    if (relative_x < 0) {
        editor->cursor.pos_in_text = line.start;
        editor->cursor.line = clicked_line;
        editor->cursor.pos_in_line = 0;
        return;
    }

    // Clamp col to line length
    if (clicked_col < 0) clicked_col = 0;
    if (clicked_col > line_length) clicked_col = line_length;

    editor->cursor.pos_in_text = line.start + clicked_col;
    editor->cursor.line = clicked_line;
    editor->cursor.pos_in_line = clicked_col;
}



void draw_char(char c, float x, float y, float scale) {
    if (c < 32 || c > 126) return; // Only printable ASCII
    int idx = c - 32;
    for (int row = 0; row < FONT_HEIGHT; ++row) {
        unsigned char bits = font[idx][row];
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

void renderTextScrolled(char* text, float x, float y, float scale, Scroll *scroll, Vec4f color) {
    glColor4f(color.x, color.y, color.z, color.w);
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
    // Offset for line numbers: 5 columns worth of width
    float line_number_offset = LINE_NUMS_OFFSET;
    float x = line_number_offset + (editor->cursor.pos_in_line - scroll->x_offset) * FONT_WIDTH * scale;
    float y = 10 + (editor->cursor.line - scroll->y_offset) * FONT_HEIGHT * scale;
    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x, y + (FONT_HEIGHT * scale));
    glEnd();
}

void editor_recalculate_cursor_pos(Editor* editor){
    int target_line = editor->cursor.line;
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
}

void editor_recalc_cursor_pos_and_line(Editor* editor){
    int pos = editor->cursor.pos_in_text;
    int line = 0;
    int col = 0;

    for (int i = 0; i < pos && editor->text.data[i]; ++i) {
        if (editor->text.data[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
    editor->cursor.line = line;
    editor->cursor.pos_in_line = col;
}


void renderText(char* text, float x, float y, float scale, Vec4f color) {
    glColor4f(color.x, color.y, color.z, color.w);
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

    float scale_prompt = 2.0f;
    int prompt_width = strlen(prompt) * FONT_WIDTH * scale_prompt;

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
    // prompt
    float prompt_x = x + 10;
    float prompt_y = y - 40;
    renderText(prompt, prompt_x, prompt_y, scale_prompt, WHITE);

    // command text
    float cmd_x = prompt_x + prompt_width;
    renderText(editor->command_text.data, cmd_x, prompt_y, scale_prompt, WHITE);

    // cursor
    int cx = cmd_x + editor->command_cursor.pos_in_text * FONT_WIDTH * scale_prompt;
    int cy = prompt_y;

    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
    glVertex2f(cx, cy);
    glVertex2f(cx, cy + (FONT_HEIGHT * scale_prompt));
    glEnd();
}

void render_scrollbar(Editor *editor, float scale) {
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);

    // Calculate total lines in text
    int total_lines = 1;
    for (int i = 0; editor->text.data[i]; ++i) {
        if (editor->text.data[i] == '\n') total_lines++;
    }

    int lines_on_screen = h / (FONT_HEIGHT * scale);

    // Scrollbar dimensions
    float bar_width = 12.0f;          // maybe make it wider in the future
    float bar_x = w - bar_width - 2;
    float bar_y = 10.0f;
    float bar_height = h - 20.0f;

    // Thumb size and position
    float thumb_height = (total_lines > 0) ? (bar_height * lines_on_screen) / total_lines : bar_height;
    if (thumb_height < 20.0f) thumb_height = 20.0f;
    if (thumb_height > bar_height) thumb_height = bar_height;

    float max_offset = (total_lines > lines_on_screen) ? (total_lines - lines_on_screen) : 1;
    float thumb_y = bar_y;
    if (max_offset > 0)
        thumb_y += (bar_height - thumb_height) * ((float)editor->scroll.y_offset / max_offset);

    // Draw scrollbar background
    glColor4f(0.18f, 0.18f, 0.18f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(bar_x, bar_y);
    glVertex2f(bar_x + bar_width, bar_y);
    glVertex2f(bar_x + bar_width, bar_y + bar_height);
    glVertex2f(bar_x, bar_y + bar_height);
    glEnd();

    // Draw thumb
    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(bar_x + 2, thumb_y);
    glVertex2f(bar_x + bar_width - 2, thumb_y);
    glVertex2f(bar_x + bar_width - 2, thumb_y + thumb_height);
    glVertex2f(bar_x + 2, thumb_y + thumb_height);
    glEnd();
}

void editor_recalculate_lines(Editor *editor){
    if (!editor->lines.lines) {
        size_t capacity = 64;
        editor->lines.lines = malloc(capacity * sizeof(Line));
        editor->lines.capacity = capacity;
    }
    editor->lines.size = 0;

    size_t start = 0;
    for (size_t i = 0; i <= editor->text.size; ++i) {
        if (editor->text.data[i] == '\n' || editor->text.data[i] == '\0') {
            if (editor->lines.size >= editor->lines.capacity) {
                editor->lines.capacity *= 2;
                editor->lines.lines = realloc(editor->lines.lines, editor->lines.capacity * sizeof(Line));
            }
            editor->lines.lines[editor->lines.size].start = start;
            editor->lines.lines[editor->lines.size].end = i;
            editor->lines.size++;
            start = i + 1;
        }
    }
}

void render_line_numbers(Editor *editor, float scale) {
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);

    int lines_on_screen = h / (FONT_HEIGHT * scale);
    int first_line = editor->scroll.y_offset;
    int last_line = first_line + lines_on_screen;
    if (last_line > (int)editor->lines.size) last_line = editor->lines.size;

    float x = 10.0f;     // TODO: large line numbers go into the text
    float y = 10.0f;     // have the gap dynamically resize based on how many lines text has

    char buf[16]; // should be plenty of lines

    for (int i = first_line; i < last_line; ++i) {
        if (i >= editor->lines.size){
            break;
        }
        snprintf(buf, sizeof(buf), "%3d", i + 1);
        if (i == editor->cursor.line){
            renderText(buf, x, y + (i - first_line) * FONT_HEIGHT * scale, scale, vec4f(1.0f, 1.0f, 1.0f, 1.0f));
        } else{
            renderText(buf, x, y + (i - first_line) * FONT_HEIGHT * scale, scale, vec4f(1.0f, 1.0f, 1.0f, 0.4f));
        }
    }
}

void format_size(long bytes, char *out, size_t out_size) {
    if (bytes < 1024) {
        snprintf(out, out_size, "%ld", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(out, out_size, "%.1fK", bytes / 1024.0);
    } else if (bytes < 1024L * 1024L * 1024L) {
        snprintf(out, out_size, "%.1fM", bytes / (1024.0 * 1024.0));
    } else {
        snprintf(out, out_size, "%.1fG", bytes / (1024.0 * 1024.0 * 1024.0));
    }
}
void render_file_browser(Editor *editor, File_Browser *fb) {
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);

    float x = 50;
    float y = 20 + (FONT_HEIGHT * fb->scale);

    char name_buf[PATH_MAX];
    char info_buf[128];

    renderText(fb->relative_path.data, 10, 10, fb->scale, WHITE); // this is one place where a freetype font would 100% look better

    // Calculate max width for size column
    int max_size_width = 0;
    for (int i = 0; i < fb->items.count; i++) {
        FB_item *item = &fb->items.items[i];
        char size_str[32];
        if (item->type == DT_DIR) {
            strcpy(size_str, "DIR");
        } else {
            format_size((long)item->data.st_size, size_str, sizeof(size_str));
        }
        int width = strlen(size_str);
        if (width > max_size_width) max_size_width = width;
    }

    float name_x = x + 400;
    for (int i = 0; i < fb->items.count; i++) {
        FB_item *item = &fb->items.items[i];

        char size_str[32];
        if (item->type == DT_DIR) {
            strcpy(size_str, "DIR");
        } else {
            format_size((long)item->data.st_size, size_str, sizeof(size_str));
        }

        // Format last modified time
        struct tm *mtm = localtime(&item->data.st_mtime);
        char time_str[32];
        if (mtm) {
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", mtm);
        } else {
            strcpy(time_str, "----");
        }


        char size_padded[40];
        snprintf(size_padded, sizeof(size_padded), "%-*s", max_size_width, size_str);

        snprintf(info_buf, sizeof(info_buf), "%s  %s", size_padded, time_str);

        if (fb->cursor == i && !fb->renaming) {
            float bx = name_x + strlen(item->name) * FONT_WIDTH * fb->scale;
            float by = y + FONT_HEIGHT * fb->scale;
            glColor4f(0.2f, 0.5f, 1.0f, 0.3f); // semi-transparent blue
            glBegin(GL_QUADS);
                glVertex2f(x, y);
                glVertex2f(bx, y);
                glVertex2f(bx, by);
                glVertex2f(x, by);
            glEnd();

        }
        
        // Draw info (size, time)
        renderText(info_buf, x, y, fb->scale, WHITE);
        
        // Draw name (after info)
        if(fb->renaming && fb->cursor == i){
            glColor4f(0.240, 0.240, 0.240, 1.0f);
            glBegin(GL_QUADS);
                glVertex2f(name_x - 5, y - 5);
                glVertex2f(name_x + (FONT_WIDTH * fb->scale) * 10, y - 5);
                glVertex2f(name_x + (FONT_WIDTH * fb->scale) * 10, y + FONT_HEIGHT * fb->scale);
                glVertex2f(name_x - 5, y + FONT_HEIGHT * fb->scale);
            glEnd();
            renderText(fb->new_file_path.data, name_x, y, fb->scale, WHITE);
        }else{
            renderText(item->name, name_x, y, fb->scale, WHITE);
        }
        

        y += FONT_HEIGHT * fb->scale + 10;

        
    }
    if(fb->new_file){
        glColor4f(0.240, 0.240, 0.240, 1.0f);
        glBegin(GL_QUADS);
            glVertex2f(name_x, y);
            glVertex2f(name_x + (FONT_WIDTH * fb->scale) * 10, y);
            glVertex2f(name_x + (FONT_WIDTH * fb->scale) * 10, y + FONT_HEIGHT * fb->scale);
            glVertex2f(name_x, y + FONT_HEIGHT * fb->scale);
        glEnd();
        renderText(fb->new_file_path.data, name_x + 5, y + 3, fb->scale - 0.1, WHITE);
    }
}

int main(int argc, char *argv[]) {
    Editor editor = {.cursor = {0}, 
                    .file_path = "", 
                    .text = strung_init(""), 
                    .command_text = strung_init(""), 
                    .command_cursor = {0},
                    .lines = {0}    
    };

    char buffer[PATH_MAX];
    if(!(realpath(".", buffer))) fprintf(stderr, "Failed, A lot (at opening init directory)\n");
    File_Browser fb = {.relative_path = strung_init(buffer), .scale = 1.5f, .new_file_path = strung_init("")};
    strung_append_char(&fb.relative_path, '/');


    bool line_switch = false;

    if (argc > 1){
        open_file(&editor, argv[1]);
        editor_recalculate_lines(&editor);
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

    float scale = 1.0f;

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE); 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }else if (event.type == SDL_DROPFILE){ 
                if(fb.file_browser){
                    move_file_to_fb(&fb, event.drop.file); // make this work with folders
                }else{
                    open_file(&editor, event.drop.file); 
                }
                SDL_free(event.drop.file);                                                               
            }
            else if (event.type == SDL_TEXTINPUT) {
                if(fb.file_browser){ 
                    if(fb.new_file || fb.renaming){
                        strung_append(&fb.new_file_path, event.text.text);
                    } 
                } else {
                    if(editor.selection){
                        strung_delete_range(&editor.text, editor.selection_start, editor.selection_end);
                        editor.cursor.pos_in_line -= editor.selection_end - editor.selection_start;
                        editor.cursor.pos_in_text -= editor.selection_end - editor.selection_start;
                        editor.selection_start = 0;
                        editor.selection_end = 0;
                        editor.selection = false;
                    }
                    if (!(SDL_GetModState() & KMOD_CTRL) && !editor.in_command) {
                        strung_insert_string(&editor.text, event.text.text, editor.cursor.pos_in_text);
                        editor.cursor.pos_in_text += strlen(event.text.text);
                        editor.cursor.pos_in_line += strlen(event.text.text);
                    } else if(editor.in_command){
                        if(!(SDL_GetModState() & KMOD_CTRL)){
                            strung_insert_string(&editor.command_text, event.text.text, editor.command_cursor.pos_in_text);
                            editor.command_cursor.pos_in_text += strlen(event.text.text);
                        }
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (fb.file_browser){
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_F2:
                        fb.file_browser = false;
                        break;
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_UP:
                        if (fb.cursor > 0 && !fb.renaming) fb.cursor--;
                        break;
                    case SDLK_DOWN:
                        if(fb.cursor < fb.items.count - 1 && !fb.renaming) fb.cursor++;
                        break;
                    case SDLK_BACKSPACE:
                        if(fb.new_file || fb.renaming) strung_remove_char(&fb.new_file_path, fb.new_file_path.size - 1);
                        break;
                    case SDLK_RETURN:
                        if(fb.new_file){
                            Strung final = strung_init("");
                            strung_append(&final, fb.relative_path.data);
                            strung_append(&final, fb.new_file_path.data);
                            if(fb.new_dir){
                                mkdir(final.data, S_IRWXU);
                            }else{
                                create_new_file(final.data);
                            }
                            read_entire_dir(&fb);
                            fb.new_file = false;
                            fb.new_dir = false;
                        } else if(fb.renaming){
                            Strung new_name_path = strung_init("");
                            strung_append(&new_name_path, fb.relative_path.data);
                            strung_append(&new_name_path, fb.new_file_path.data);

                            Strung old_name_path = strung_init("");
                            strung_append(&old_name_path, fb.relative_path.data);
                            strung_append(&old_name_path, fb.items.items[fb.cursor].name);

                            if(rename(old_name_path.data, new_name_path.data) != 0) fprintf(stderr, "Failed to rename\n");
                            read_entire_dir(&fb);
                            fb.renaming = false;
                        }else{
                            
                            char *selected = fb.items.items[fb.cursor].name;
                            if (fb.items.items[fb.cursor].type == DT_REG) {
                                // Build full path
                                char full_path[PATH_MAX];
                                snprintf(full_path, sizeof(full_path), "%s%s", fb.relative_path.data, selected);
                                char resolved[PATH_MAX];
                                if (!realpath(full_path, resolved)) {
                                    fprintf(stderr, "Failed to get full file path of %s\n", selected);
                                } else {
                                    open_file(&editor, resolved);
                                    ensure_cursor_visible(&editor, &editor.scroll, scale);
                                    fb.file_browser = false;
                                }
                            } else if (fb.items.items[fb.cursor].type == DT_DIR) {
                                char *selected = fb.items.items[fb.cursor].name;
                                if (strcmp(selected, ".") == 0) {
                                    // Stay in current directory, do nothing
                                } else if (strcmp(selected, "..") == 0) {
                                    // Go up one directory
                                    size_t len = strlen(fb.relative_path.data);
                                    if (len > 1) {
                                        // Remove trailing slash if present
                                        if (fb.relative_path.data[len - 1] == '/')
                                            fb.relative_path.data[--len] = '\0';
                                        // Find previous slash
                                        char *slash = strrchr(fb.relative_path.data, '/');
                                        if (slash && slash != fb.relative_path.data) {
                                            *slash = '\0';
                                            len = slash - fb.relative_path.data;
                                        } else {
                                            // Already at root, stay
                                            fb.relative_path.data[0] = '/';
                                            fb.relative_path.data[1] = '\0';
                                            len = 1;
                                        }
                                    }
                                    // Ensure trailing slash except for root
                                    if (strcmp(fb.relative_path.data, "/") != 0) {
                                        if (fb.relative_path.data[len - 1] != '/') {
                                            fb.relative_path.data[len] = '/';
                                            fb.relative_path.data[len + 1] = '\0';
                                        }
                                    }
                                    // Truncate any extra data after the new end
                                    fb.relative_path.size = strlen(fb.relative_path.data);
                                } else {
                                    // Go into selected directory
                                    size_t len = strlen(fb.relative_path.data);
                                    if (len > 0 && fb.relative_path.data[len - 1] != '/')
                                        strung_append_char(&fb.relative_path, '/');
                                    strung_append(&fb.relative_path, selected);
                                    strung_append_char(&fb.relative_path, '/');
                                }
                                read_entire_dir(&fb);
                                fb.cursor = 0;
                            }
                        }
                        break;
                    case SDLK_DELETE:
                        Strung final_path = strung_init("");
                        strung_append(&final_path, fb.relative_path.data);
                        strung_append(&final_path, fb.items.items[fb.cursor].name);
                        if(!(strcmp(fb.items.items[fb.cursor].name, "..") == 0 || strcmp(fb.items.items[fb.cursor].name, ".") == 0)){
                            if(fb.items.items[fb.cursor].type == DT_DIR){
                                rmdir(final_path.data);
                            }else if(fb.items.items[fb.cursor].type == DT_REG){
                                remove(final_path.data);
                            } else{
                                fprintf(stderr, "Invalid file type to delete\n");
                            }
                        }else{
                            fprintf(stderr, "We do not support deleting current dir or prev dir, for safety reasons\n");
                            fprintf(stderr, "and because i don't know what will happen\n");
                        }
                        read_entire_dir(&fb);
                    case SDLK_MINUS:
                        if (event.key.keysym.mod & KMOD_CTRL) fb.scale -= 0.5;
                        break;
                    case SDLK_EQUALS:
                        if (event.key.keysym.mod & KMOD_CTRL) fb.scale += 0.5;
                        break;
                    case SDLK_n:
                        if(event.key.keysym.mod & KMOD_CTRL){
                            if(event.key.keysym.mod & KMOD_SHIFT){
                                fb.new_dir = true;
                            }
                            strung_reset(&fb.new_file_path);
                            fb.new_file = true;
                        }
                        break;
                    case SDLK_r:
                        if(event.key.keysym.mod & KMOD_CTRL){
                            strung_reset(&fb.new_file_path);
                            strung_append(&fb.new_file_path, fb.items.items[fb.cursor].name);
                            fb.renaming = true;
                        }
                    default:
                        break;
                    }
                }
                else {
                    switch (event.key.keysym.sym) {
                        case SDLK_BACKSPACE:
                            if (editor.in_command){
                                if(editor.command_cursor.pos_in_text > 0){
                                    strung_remove_char(&editor.command_text, editor.command_cursor.pos_in_text - 1);
                                    editor.command_cursor.pos_in_text--;
                                }
                            }
                            else if (editor.cursor.pos_in_text > 0) {

                                if (editor.selection) {
                                    if (editor.selection_end < editor.selection_start) {
                                        int temp = editor.selection_end;
                                        editor.selection_end = editor.selection_start;
                                        editor.selection_start = temp;
                                    }
                                    strung_delete_range(&editor.text, editor.selection_start, editor.selection_end);
                                    editor.cursor.pos_in_text = editor.selection_start;
                                    editor.selection = false;
                                    editor.selection_start = 0;
                                    editor.selection_end = 0;
                                    editor_recalc_cursor_pos_and_line(&editor);
                                }

                                else if (editor.text.data[editor.cursor.pos_in_text - 1] == '\n') {
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
                                    editor_recalculate_lines(&editor);
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
                                editor_recalculate_lines(&editor); // Can optimize a bit if i recalculate only when deleting newline
                            }
                            break;
                        case SDLK_RETURN:
                            strung_insert_char(&editor.text, '\n', editor.cursor.pos_in_text);
                            editor.cursor.pos_in_text++;
                            editor.cursor.line++;
                            editor.cursor.pos_in_line = 0;
                            editor_recalculate_lines(&editor);
                            break;
                        case SDLK_TAB:
                            strung_insert_string(&editor.text, "    ", editor.cursor.pos_in_text);
                            editor.cursor.pos_in_line += 4;
                            editor.cursor.pos_in_text += 4;
                            break;
                        case SDLK_HOME: 
                            // Move cursor to the start of the current line
                            {
                                editor.cursor.pos_in_text = editor.lines.lines[editor.cursor.line].start;
                                editor.cursor.pos_in_line = 0;
                            }
                            break;
                        case SDLK_END:
                            {
                                editor.cursor.pos_in_text = editor.lines.lines[editor.cursor.line].end;
                                editor.cursor.pos_in_line = editor.lines.lines[editor.cursor.line].end - editor.lines.lines[editor.cursor.line].start;
                            }
                            break;
                        case SDLK_F2:
                            read_entire_dir(&fb);
                            fb.file_browser = true;
                            break;
                        case SDLK_F3:
                            strung_reset(&editor.command_text);
                            editor.command_cursor.pos_in_text = 0;
                            editor.in_command = !editor.in_command;
                            break;
                        case SDLK_LEFT:
                            if (editor.in_command) {
                                if (editor.command_cursor.pos_in_text > 0) {
                                    editor.command_cursor.pos_in_text--;
                                }
                            } else {
                                bool shift = (event.key.keysym.mod & KMOD_SHIFT);
                                bool ctrl = (event.key.keysym.mod & KMOD_CTRL);
                                if (shift) {
                                    if (!editor.selection) {
                                        editor.selection = true;
                                        editor.selection_start = editor.cursor.pos_in_text;
                                    }
                                } else {
                                    editor.selection_start = 0;
                                    editor.selection_end = 0;
                                    editor.selection = false;
                                }
                                if (ctrl) {
                                    // Move left to previous space or newline
                                    int pos = editor.cursor.pos_in_text;
                                    if (pos > 0) {
                                        pos--;
                                        // skip over current spaces or \n
                                        while (pos > 0 && (editor.text.data[pos] == ' ' || editor.text.data[pos] == '\n')) {
                                            pos--;
                                        }

                                        while (pos > 0 && editor.text.data[pos] != ' ' && editor.text.data[pos] != '\n') { // move to previous space or \n
                                            pos--;
                                        }
                                        // If we stopped at a space or \n move after it unless at start
                                        if (pos > 0) pos++;
                                        editor.cursor.pos_in_text = pos;
                                        // Recalculate line and col
                                        editor_recalc_cursor_pos_and_line(&editor);
                                        ensure_cursor_visible(&editor, &editor.scroll, scale);
                                    }
                                } else {
                                    // If at start of line, move to end of previous line
                                    int pos = editor.cursor.pos_in_text;
                                    if (pos > 0 && editor.cursor.pos_in_line == 0) {
                                        pos--; // move to previous character (should be '\n')
                                        int col = 0;
                                        // Count backwards to previous '\n' or start
                                        int scan = pos;
                                        while (scan > 0 && editor.text.data[scan - 1] != '\n') {
                                            scan--;
                                            col++;
                                        }
                                        editor.cursor.pos_in_text = pos;
                                        editor.cursor.line--;
                                        editor.cursor.pos_in_line = col;
                                    } else if (editor.text.data[editor.cursor.pos_in_text-1] == '\n') {
                                        editor.cursor.line--;
                                        editor_recalculate_cursor_pos(&editor);
                                        ensure_cursor_visible(&editor, &editor.scroll, scale);
                                    } else if (editor.cursor.pos_in_text > 0) {
                                        editor.cursor.pos_in_text--;
                                        if (editor.cursor.pos_in_line > 0) {
                                            editor.cursor.pos_in_line--;
                                        }
                                        ensure_cursor_visible(&editor, &editor.scroll, scale);
                                    }
                                }
                                if (shift) {
                                    editor.selection_end = editor.cursor.pos_in_text;
                                }
                            }
                            break;
                        case SDLK_RIGHT:
                            if (editor.in_command) {
                                if (editor.command_cursor.pos_in_text < editor.command_text.size) {
                                    editor.command_cursor.pos_in_text++;
                                }
                            } else {
                                bool shift = (event.key.keysym.mod & KMOD_SHIFT);
                                bool ctrl = (event.key.keysym.mod & KMOD_CTRL);
                                if (shift) {
                                    if (!editor.selection) {
                                        editor.selection = true;
                                        editor.selection_start = editor.cursor.pos_in_text;
                                    }
                                } else {
                                    editor.selection_start = 0;
                                    editor.selection_end = 0;
                                    editor.selection = false;
                                }
                                if (ctrl) {
                                    int pos = editor.cursor.pos_in_text;
                                    int len = editor.text.size;
                                    while (pos < len && (editor.text.data[pos] == ' ' || editor.text.data[pos] == '\n')) { // Skip over any current spaces/newline
                                        pos++;
                                    }

                                    while (pos < len && editor.text.data[pos] != ' ' && editor.text.data[pos] != '\n') { // Move to next space/newline or end
                                        pos++;
                                    }
                                    editor.cursor.pos_in_text = pos;
                                    editor_recalc_cursor_pos_and_line(&editor);
                                    ensure_cursor_visible(&editor, &editor.scroll, scale);
                                } else {
                                    if (editor.text.data[editor.cursor.pos_in_text] == '\n') {
                                        editor.cursor.pos_in_line = 0;
                                        editor.cursor.line++;
                                        editor.cursor.pos_in_text++;
                                        ensure_cursor_visible(&editor, &editor.scroll, scale);
                                    } else if (editor.cursor.pos_in_text < editor.text.size) {
                                        editor.cursor.pos_in_text++;
                                        editor.cursor.pos_in_line++;
                                        ensure_cursor_visible(&editor, &editor.scroll, scale);
                                    }
                                }
                                if (shift) {
                                    editor.selection_end = editor.cursor.pos_in_text;
                                }
                            }
                            break;
                        case SDLK_UP:
                            if (editor.cursor.line > 0) {
                                bool shift = (event.key.keysym.mod & KMOD_SHIFT);
                                if (shift) {
                                    if (!editor.selection) {
                                        editor.selection = true;
                                        editor.selection_start = editor.cursor.pos_in_text;
                                    }
                                } else {
                                    editor.selection_start = 0;
                                    editor.selection_end = 0;
                                    editor.selection = false;
                                }
                                editor.cursor.line--;
                                editor_recalculate_cursor_pos(&editor);
                                if (shift) {
                                    editor.selection_end = editor.cursor.pos_in_text;
                                }
                            }
                            ensure_cursor_visible(&editor, &editor.scroll, scale);
                            break;
                        case SDLK_DOWN:

                            bool shift = (event.key.keysym.mod & KMOD_SHIFT);
                            if (shift) {
                                if (!editor.selection) {
                                    editor.selection = true;
                                    editor.selection_start = editor.cursor.pos_in_text;
                                }
                            } else {
                                editor.selection_start = 0;
                                editor.selection_end = 0;
                                editor.selection = false;
                            }
                            editor.cursor.line++;
                            editor_recalculate_cursor_pos(&editor);
                            if (shift) {
                                editor.selection_end = editor.cursor.pos_in_text;
                            }

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
                        case SDLK_a:
                            if(event.key.keysym.mod & KMOD_CTRL){
                                editor.selection = true;
                                editor.selection_start = 0;
                                editor.selection_end = editor.text.size;
                            }
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
                        case SDLK_x:
                            if(event.key.keysym.mod & KMOD_CTRL){
                                if(editor.selection){
                                    char* selected = strung_substr(&editor.text, editor.selection_start, editor.selection_end - editor.selection_start);
                                    if(SDL_SetClipboardText(selected) < 0){
                                        fprintf(stderr, "%s could not be copied to clipboard\n", selected);
                                    }
                                    strung_delete_range(&editor.text, editor.selection_start, editor.selection_end);
                                    editor.selection_start = 0;
                                    editor.selection_end = 0;
                                    editor.selection = false;
                                } else {}                            
                            }

                        case SDLK_c:
                            if(event.key.keysym.mod & KMOD_CTRL){
                                if(editor.selection){
                                    if(editor.selection_end < editor.selection_start){
                                        SEL_SWAP(editor.selection_start, editor.selection_end)
                                    }
                                    char* selected = strung_substr(&editor.text, editor.selection_start, editor.selection_end - editor.selection_start);
                                    if(SDL_SetClipboardText(selected) < 0){
                                        fprintf(stderr, "%s could not be copied to clipboard\n", selected);
                                    }
                                } else {}
                            }
                            break;
                        case SDLK_v:
                            if(event.key.keysym.mod & KMOD_CTRL){
                                char* text = SDL_GetClipboardText();
                                strung_insert_string(&editor.text, text, editor.cursor.pos_in_text);
                                editor.cursor.pos_in_line += strlen(text);
                                editor.cursor.pos_in_text += strlen(text);
                                SDL_free(text);
                                editor_recalculate_lines(&editor);
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
            }
            } else if (event.type == SDL_MOUSEWHEEL) {
                editor.scroll.y_offset -= event.wheel.y * 10;
                clamp_scroll(&editor, &editor.scroll, scale);
            } else if(event.type == SDL_MOUSEBUTTONDOWN){
                if (event.button.button == SDL_BUTTON_LEFT && !editor.in_command) {

                // Check if mouse is over the scrollbar
                int win_w, win_h;
                SDL_GetWindowSize(editor.window, &win_w, &win_h);
                float bar_width = 12.0f;
                float bar_x = win_w - bar_width - 2;
                float bar_y = 10.0f;
                float bar_height = win_h - 20.0f;

                if (event.button.x >= bar_x && event.button.x <= bar_x + bar_width &&
                    event.button.y >= bar_y && event.button.y <= bar_y + bar_height) {
                    int total_lines = 1;
                    for (int i = 0; editor.text.data[i]; ++i) {
                        if (editor.text.data[i] == '\n') total_lines++;
                    }
                    int lines_on_screen = win_h / (FONT_HEIGHT * scale);
                    float thumb_height = (total_lines > 0) ? (bar_height * lines_on_screen) / total_lines : bar_height;
                    if (thumb_height < 20.0f) thumb_height = 20.0f;
                    if (thumb_height > bar_height) thumb_height = bar_height;
                    float max_offset = (total_lines > lines_on_screen) ? (total_lines - lines_on_screen) : 1;

                    // Calculate where the click happened relative to the bar
                    float click_y = event.button.y - bar_y;
                    float thumb_y = 0.0f;
                    if (max_offset > 0)
                        thumb_y = (bar_height - thumb_height) * ((float)editor.scroll.y_offset / max_offset);

                    // If click is on the thumb, start dragging
                    if (click_y >= thumb_y && click_y <= thumb_y + thumb_height) {
                        bool dragging = true;
                        int drag_offset = click_y - thumb_y;
                        // Save current GL context and window
                        SDL_Window* win = editor.window;
                        SDL_GLContext ctx = SDL_GL_GetCurrentContext();

                        while (dragging) {
                            SDL_Event drag_event;
                            // Use SDL_WaitEventTimeout to avoid busy loop and allow redraw
                            if (SDL_WaitEventTimeout(&drag_event, 10)) {
                                if (drag_event.type == SDL_MOUSEBUTTONUP && drag_event.button.button == SDL_BUTTON_LEFT) {
                                    dragging = false;
                                    break;
                                } else if (drag_event.type == SDL_MOUSEMOTION) {
                                    float new_thumb_y = drag_event.motion.y - bar_y - drag_offset;
                                    if (new_thumb_y < 0) new_thumb_y = 0;
                                    if (new_thumb_y > bar_height - thumb_height) new_thumb_y = bar_height - thumb_height;
                                    int new_offset = (int)((new_thumb_y / (bar_height - thumb_height)) * max_offset + 0.5f);
                                    if (new_offset < 0) new_offset = 0;
                                    if (new_offset > max_offset) new_offset = max_offset;
                                    editor.scroll.y_offset = new_offset;
                                    clamp_scroll(&editor, &editor.scroll, scale);

                                    // Redraw immediately while dragging
                                    int w, h;
                                    SDL_GetWindowSize(editor.window, &w, &h);
                                    glViewport(0, 0, w, h);
                                    glClear(GL_COLOR_BUFFER_BIT);
                                    glMatrixMode(GL_PROJECTION);
                                    glLoadIdentity();
                                    glOrtho(0, w, h, 0, -1.0, 1.0);
                                    glMatrixMode(GL_MODELVIEW);
                                    glLoadIdentity();

                                    renderTextScrolled(editor.text.data, (FONT_WIDTH * scale) * 3 + 10.0f, 10.0f, scale, &editor.scroll, WHITE);
                                    render_scrollbar(&editor, scale);
                                    render_line_numbers(&editor, scale);

                                    if(editor.in_command){
                                        char buffer[100];
                                        render_text_box(&editor, buffer, "Enter Command: ", scale);
                                    }else{
                                        renderCursorScrolled(&editor, scale, &editor.scroll);
                                    }

                                    render_selection(&editor, scale, &editor.scroll);

                                    SDL_GL_SwapWindow(editor.window);
                                } else if (drag_event.type == SDL_QUIT) {
                                    dragging = false;
                                    running = false;
                                    break;
                                }
                            }
                        }
                        // Don't move cursor or select text if scrollbar was dragged
                        break;
                    } else {
                        // Clicked on scrollbar background: scroll to that spot
                        float thumb_center = thumb_height / 2.0f;
                        float new_thumb_y = click_y - thumb_center;
                        if (new_thumb_y < 0) new_thumb_y = 0;
                        if (new_thumb_y > bar_height - thumb_height) new_thumb_y = bar_height - thumb_height;
                        int new_offset = (int)((new_thumb_y / (bar_height - thumb_height)) * max_offset + 0.5f);
                        if (new_offset < 0) new_offset = 0;
                        if (new_offset > max_offset) new_offset = max_offset;
                        editor.scroll.y_offset = new_offset;
                        clamp_scroll(&editor, &editor.scroll, scale);
                        // Don't move cursor or select text if scrollbar was clicked
                        break;
                    }
                }

                
                static Uint32 last_click_time = 0;
                static int last_click_x = 0, last_click_y = 0;
                Uint32 now = SDL_GetTicks();
                int double_click_distance = 5; // pixels

                if (event.button.button == SDL_BUTTON_LEFT && !editor.in_command) {        
                    bool is_double_click = (now - last_click_time < 500 &&
                        abs(event.button.x - last_click_x) < double_click_distance &&
                        abs(event.button.y - last_click_y) < double_click_distance);

                    if (is_double_click) {
                        editor_move_cursor_to_click(&editor, event.button.x, event.button.y, scale);

                        // Find word boundaries
                        int start = editor.cursor.pos_in_text;
                        int end = editor.cursor.pos_in_text;
                        char *data = editor.text.data;
                        int len = editor.text.size;

                        // Move start left to word boundary
                        while (start > 0 && ((data[start-1] >= 'a' && data[start-1] <= 'z') ||
                                             (data[start-1] >= 'A' && data[start-1] <= 'Z') ||      // double clicking highlights word
                                             (data[start-1] >= '0' && data[start-1] <= '9') ||
                                             data[start-1] == '_')) {
                            start--;
                        }
                        // Move end right to word boundary
                        while (end < len && ((data[end] >= 'a' && data[end] <= 'z') ||
                                             (data[end] >= 'A' && data[end] <= 'Z') ||
                                             (data[end] >= '0' && data[end] <= '9') ||
                                             data[end] == '_')) {
                            end++;
                        }
                        editor.selection = true;
                        editor.selection_start = start;
                        editor.selection_end = end;
                        editor.cursor.pos_in_text = end;
                        editor_recalc_cursor_pos_and_line(&editor);
                    } else {
                        // Single click: just move cursor, do not select
                        editor.selection = false;
                        editor.selection_start = 0;
                        editor.selection_end = 0;
                        editor_move_cursor_to_click(&editor, event.button.x, event.button.y, scale);
                    }
                    last_click_time = now;
                    last_click_x = event.button.x;
                    last_click_y = event.button.y;
                }
            }
        }

        }


        int w, h;
        SDL_GetWindowSize(editor.window, &w, &h);
        
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        if(fb.file_browser){
            render_file_browser(&editor, &fb);
        }
        else {
            renderTextScrolled(editor.text.data, LINE_NUMS_OFFSET, 10.0f, scale, &editor.scroll, WHITE);
            render_scrollbar(&editor, scale);
            render_line_numbers(&editor, scale);
            render_selection(&editor, scale, &editor.scroll);
        }
        
        if(editor.in_command){
            char buffer[100];
            render_text_box(&editor, buffer, "Enter Command: ", scale);
        }else{
            if(!fb.file_browser) renderCursorScrolled(&editor, scale, &editor.scroll);
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

