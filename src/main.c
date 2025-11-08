#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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

 
#define CTRL_HELD (event.key.keysym.mod & KMOD_CTRL)
#define SHIFT_HELD (event.key.keysym.mod & KMOD_SHIFT)


typedef struct {
    GLuint texture;
    int width, height;
    int advance;
    int minx, maxx, miny, maxy;
} Glyph;

#define GLYPH_CACHE_SIZE 256
Glyph* glyph_cache[GLYPH_CACHE_SIZE]; // ASCII range for now


void cache_glyph(char c, TTF_Font* font) {
    if (glyph_cache[(int)c]) return;

    Glyph* g = malloc(sizeof(Glyph));
    if (!g) return;

    char str[2] = {c, '\0'};
    SDL_Color white = {255, 255, 255, 255};

    SDL_Surface* raw = TTF_RenderText_Blended(font, str, white);
    if (!raw) {
        free(g);
        return;
    }

    // Convert to RGBA32 to avoid format issues
    SDL_Surface* surface = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    if (!surface) {
        free(g);
        return;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

    g->texture = tex;
    g->width = surface->w;
    g->height = surface->h;

    TTF_GlyphMetrics(font, c, &g->minx, &g->maxx, &g->miny, &g->maxy, &g->advance);
    glyph_cache[(int)c] = g;

    SDL_FreeSurface(surface);
}



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

#define MAX_HISTORY 100

typedef struct {
    int pos_in_text;
    int pos_in_line;
    int line;
}Cursor;


typedef struct {
    Strung text;
    Cursor cursor;
} HistoryEntry;



typedef struct {
    HistoryEntry* history[MAX_HISTORY];
    int top;
} Stack;

void stack_push(Stack* stack, const Strung* text, Cursor cursor) {
    if (stack->top >= MAX_HISTORY) return;

    HistoryEntry* entry = malloc(sizeof(HistoryEntry));
    entry->text = strung_copy(text);
    entry->cursor = cursor;

    stack->history[stack->top++] = entry;
}


HistoryEntry* stack_pop(Stack* stack) {
    if (stack->top == 0) return NULL;
    return stack->history[--stack->top];
}


void stack_clear(Stack* stack) {
    while (stack->top > 0) {
        HistoryEntry* e = stack->history[--stack->top];
        strung_free(&e->text);
        free(e);
    }
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
    
    char* copied_file_name;
    Strung copied_file_contents;

    Strung search_buffer;

    FB_items items;
}File_Browser;

typedef struct {
    int x_offset; // horizontal 
    int y_offset; // vertical 
} Scroll;



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
    float scale;

    Scroll scroll;
    Cursor cursor;

    char* file_path;
    char* file_password;

    Strung text;
    Lines lines;

    int selection_start;
    int selection_end;
    bool selection;

    Stack undo_stack;
    Stack redo_stack;
}Editor;

void editor_recalculate_lines(Editor *editor);
void ensure_cursor_visible(Editor *editor);
void editor_center_cursor(Editor *editor);

#define WHITE vec4f(1.0f, 1.0f, 1.0f, 1.0f)
TTF_Font *font; // global font for now
bool editkeys = false;

// y'know i think i could just do like an array of modifier keys maybe?
typedef struct{
    SDL_KeyCode key;
    bool ctrl;
    bool shift;
    bool alt;

    char* description; // for the keybinds screen thing
}Keybind;

typedef struct{
    Keybind remove_char;
    Keybind delete_char;
    Keybind newline;
    Keybind indent;
    Keybind start_of_line;
    Keybind end_of_line;
    Keybind toggle_fb;
    Keybind cmdbox;
    Keybind cursor_left;
    Keybind cursor_right;
    Keybind cursor_up;
    Keybind cursor_down;
    Keybind quit_app;
    Keybind scale_up;
    Keybind scale_down;
    Keybind scale_reset;
    Keybind select_all;
    Keybind savef;
    Keybind openf;
    Keybind cut;
    Keybind copy;
    Keybind paste;
    Keybind undo;
    Keybind redo;
    Keybind scroll_up;
    Keybind scroll_down;
}Keybinds;

typedef struct{
    char* path_to_font;
    float editor_scale;

    Keybinds keybinds;
}Settings;

#include "la.c"
#include "command_box.c"
#include "lexer.c"

#include "settings.c"

#define LINE_NUMS_OFFSET (FONT_WIDTH * editor.scale) * 5.0f + 10.0f
// #define LINE_NUMS_OFFSETP (FONT_WIDTH * editor->scale) * 5.0f + 10.0f // WHY CAN'T THIS DAMN LANGAUGE JUST AUTO DE-REFERENCE


#define LINE_NUMS_OFFSETP 48  


// Returns the *scaled* horizontal advance for `ch`, including kerning against `prev` when enabled.
static inline float
ttf_advance_with_kerning(TTF_Font* font, unsigned char prev, unsigned char ch, float scale)
{
    int minx, maxx, miny, maxy, advance = 0;
    if (TTF_GlyphMetrics(font, (Uint16)ch, &minx, &maxx, &miny, &maxy, &advance) != 0) {
        int space_adv = 0, dummy;
        if (TTF_GlyphMetrics(font, (Uint16)' ', &dummy, &dummy, &dummy, &dummy, &space_adv) != 0)
            space_adv = 8;
        advance = (ch == '\t') ? (space_adv * 4) : space_adv;
    }

#if SDL_TTF_VERSION_ATLEAST(2,0,18)
    if (prev && TTF_GetFontKerning(font)) {
        int kern = TTF_GetFontKerningSizeGlyphs(font, (Uint16)prev, (Uint16)ch);
        advance += kern;
    }
#endif

    return advance * scale;
}



GLuint create_texture_from_surface(SDL_Surface* surface) {
    GLuint texture;
    GLenum format = surface->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D, 0, format,
        surface->w, surface->h,
        0, format, GL_UNSIGNED_BYTE,
        surface->pixels
    );

    return texture;
}

#define FONT_WIDTH 36
#define FONT_HEIGHT 60

void draw_char(char c, float x, float y, float scale, Vec4f color) {
    if ((unsigned char)c >= GLYPH_CACHE_SIZE) return;
    if (!glyph_cache[(int)c]) cache_glyph(c, font);  


    Glyph* g = glyph_cache[(int)c];

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g->texture);
    glColor4f(color.x, color.y, color.z, color.w);

    float w = g->width * scale;
    float h = g->height * scale;

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(x, y);
        glTexCoord2f(1, 0); glVertex2f(x + w, y);
        glTexCoord2f(1, 1); glVertex2f(x + w, y + h);
        glTexCoord2f(0, 1); glVertex2f(x, y + h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}


#if 0
void renderTextScrolled(Editor* editor, float x, float y, float scale, Vec4f color) {
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);

    int line_height = FONT_HEIGHT * scale;
    int max_lines = h / line_height;

    int scroll_y = editor->scroll.y_offset;
    int scroll_x = editor->scroll.x_offset;

    float base_x = LINE_NUMS_OFFSETP;  // Matches renderCursorScrolled
    float draw_y = y;

    int current_line = 0;
    int line_number = 0;
    const char* ptr = editor->text.data;

    // Skip lines above scroll
    while (line_number < scroll_y && *ptr != '\0') {
        if (*ptr == '\n') ++line_number;
        ++ptr;
    }

    // Draw visible lines
    while (*ptr != '\0' && current_line < max_lines) {
        float draw_x = base_x;
        int col = 0;

        while (*ptr != '\0' && *ptr != '\n') {
            char c = *ptr;

            if (!glyph_cache[(unsigned char)c]) {
                cache_glyph(c, font);
            }

            Glyph* g = glyph_cache[(unsigned char)c];
            float advance = g ? g->advance * scale : FONT_WIDTH * scale;

            // Draw only visible columns
            if (col >= scroll_x) {
                draw_char(c, draw_x, draw_y, scale, color);
                draw_x += advance;
            } else {
                // Skip column
                draw_x += advance;
            }

            ++ptr;
            ++col;
        }

        // Skip newline
        if (*ptr == '\n') ++ptr;

        draw_y += line_height;
        ++current_line;
    }
}
#else
void renderTextScrolled(Editor* editor, float x, float y, float scale, Vec4f color) {
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);
    int line_height = FONT_HEIGHT * scale;
    int max_lines = h / line_height;

    int scroll_y = editor->scroll.y_offset;
    int scroll_x = editor->scroll.x_offset;

    float base_x = LINE_NUMS_OFFSETP;
    float draw_y = y;

    const char* ptr = editor->text.data;
    int line = 0;

    // Skip to scroll_y
    while (line < scroll_y && *ptr) {
        if (*ptr++ == '\n') ++line;
    }

    int current_line = 0;
    while (*ptr && current_line < max_lines) {
        float draw_x = base_x;
        const char* line_start = ptr;

        // Find end of line
        while (*ptr && *ptr != '\n') ++ptr;
        int line_len = ptr - line_start;

        // Check for preprocessor directive
        if (is_preprocessor_directive(line_start, line_len)) {
            Vec4f color = vec4f(0.6f, 0.8f, 1.0f, 1.0f);  // cyan-ish
            for (int i = 0; i < line_len; ++i) {
                char c = line_start[i];
                if (!glyph_cache[(unsigned char)c]) cache_glyph(c, font);
                Glyph* g = glyph_cache[(unsigned char)c];
                float advance = g ? g->advance * scale : FONT_WIDTH * scale;
                if (i >= scroll_x) {
                    draw_char(c, draw_x, draw_y, scale, color);
                }
                draw_x += advance;
            }
        } else {
            // Token-based parsing
            const char* p = line_start;
            int col = 0;
            while (p < ptr) {
                Token tok = next_token(p);
                if (tok.length <= 0) break;

                Vec4f color = token_color(tok.type);
                for (int i = 0; i < tok.length; ++i) {
                    char c = tok.start[i];
                    if (!glyph_cache[(unsigned char)c]) cache_glyph(c, font);
                    Glyph* g = glyph_cache[(unsigned char)c];
                    float advance = g ? g->advance * scale : FONT_WIDTH * scale;
                    if (col >= scroll_x) {
                        draw_char(c, draw_x, draw_y, scale, color);
                    }
                    draw_x += advance;
                    col++;
                }
                p += tok.length;
            }
        }

        if (*ptr == '\n') ++ptr;
        draw_y += line_height;
        ++current_line;
    }
}

#endif


void renderText(char* text, float x, float y, float scale, Vec4f color) {
    float orig_x = x;

    for (int i = 0; text[i]; ++i) {
        if (text[i] == '\n') {
            // Advance to next line
            y += TTF_FontHeight(font) * scale;
            x = orig_x;
        } else {
            // Draw each character using TTF
            draw_char(text[i], x, y, scale, color);

            // Advance x by character width
            int minx, maxx, miny, maxy, advance;
            if (TTF_GlyphMetrics(font, text[i], &minx, &maxx, &miny, &maxy, &advance) == 0) {
                x += advance * scale;
            } else {
                x += TTF_FontHeight(font) * 0.5f * scale;  // Fallback spacing
            }
        }
    }
}

float get_line_width(Editor *editor, int index, int col_offset, float scale) {
    float width = 0;
    int col = 0;

    // Walk backwards from index to find start of line
    int start = index;
    while (start > 0 && editor->text.data[start - 1] != '\n')
        start--;

    for (int i = 0; i < col_offset; ++i) {
        char c = editor->text.data[start + i];
        if (!c || c == '\n') break;

        if (!glyph_cache[(unsigned char)c])
            cache_glyph(c, font);

        Glyph *g = glyph_cache[(unsigned char)c];
        width += g ? g->advance * scale : FONT_WIDTH * scale;
    }

    return width;
}


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

    float line_number_offset = LINE_NUMS_OFFSETP;
    int line = 0;
    float x = line_number_offset;
    float y = 10;

    for (int i = 0; i < sel_end && editor->text.data[i]; ++i) {
        char c = editor->text.data[i];

        // Update line/column state
        if (c == '\n') {
            ++line;
            x = line_number_offset;
            y += FONT_HEIGHT * scale;
            continue;
        }

        // Skip glyphs above scroll
        if (line < scroll->y_offset) continue;

        // Skip glyphs to the left of scroll
        float glyph_x = x;
        float glyph_y = y;

        if (!glyph_cache[(unsigned char)c])
            cache_glyph(c, font);

        Glyph *g = glyph_cache[(unsigned char)c];
        float advance = g ? g->advance * scale : FONT_WIDTH * scale;

        // Only draw selection if this char is within the selected range
        if (i >= sel_start && i < sel_end) {
            int visible_line = line - scroll->y_offset;
            float visible_x = glyph_x - (scroll->x_offset > 0 ? get_line_width(editor, i, scroll->x_offset, scale) : 0);

            if (visible_line >= 0) {
                float draw_y = 10 + visible_line * FONT_HEIGHT * scale;
                glColor4f(0.2f, 0.5f, 1.0f, 0.3f); // semi-transparent blue
                glBegin(GL_QUADS);
                    glVertex2f(visible_x, draw_y);
                    glVertex2f(visible_x + advance, draw_y);
                    glVertex2f(visible_x + advance, draw_y + FONT_HEIGHT * scale);
                    glVertex2f(visible_x, draw_y + FONT_HEIGHT * scale);
                glEnd();
            }
        }

        x += advance;
    }

    glColor4f(1, 1, 1, 1); // reset color
}



// Snap to next tab stop using a fixed tab cell width
static inline float
next_tab_stop(float x, float tab_width)
{
    float next = floorf(x / tab_width + 1.0f) * tab_width;
    return next;
}

void editor_move_cursor_to_click(Editor* editor, int x, int y, float scale, TTF_Font* font)
{
    if(editor->text.size < 1){
        return;
    }

    const int line_skip = (int)llroundf(TTF_FontLineSkip(font) * scale);
    const int relative_y = y - 10;
    int clicked_line = (relative_y / (line_skip > 0 ? line_skip : 1)) + editor->scroll.y_offset + 1;

    if (clicked_line < 0) clicked_line = 0;
    if (clicked_line >= (int)editor->lines.size) clicked_line = (int)editor->lines.size - 1;

    Line line = editor->lines.lines[clicked_line];
    int line_length = line.end - line.start;
    const char* line_ptr = &editor->text.data[line.start];

    float relative_x = (float)x - (float)LINE_NUMS_OFFSETP;
    if (relative_x < 0.0f) {
        editor->cursor.line = clicked_line;
        editor->cursor.pos_in_line = 0;
        editor->cursor.pos_in_text = line.start;
        return;
    }

    // Tab width (e.g., 4 spaces)
    int space_adv = 0, dummy;
    if (TTF_GlyphMetrics(font, (Uint16)' ', &dummy, &dummy, &dummy, &dummy, &space_adv) != 0)
        space_adv = 8;
    float tab_width = space_adv * 4 * scale;

    float current_x = 0.0f;
    int best_col = 0;
    float best_dist = 1e30f;
    unsigned char prev = 0;

    for (int col = 0; col <= line_length; ++col) {
        float mid_x = current_x;

        if (col < line_length) {
            unsigned char ch = (unsigned char)line_ptr[col];
            float adv;

            if (ch == '\t')
                adv = next_tab_stop(current_x, tab_width) - current_x;
            else
                adv = ttf_advance_with_kerning(font, prev, ch, scale);

            mid_x += adv * 0.5f;
        }

        float dist = fabsf(mid_x - relative_x);
        if (dist < best_dist) {
            best_dist = dist;
            best_col = col;
        }

        if (col < line_length) {
            unsigned char ch = (unsigned char)line_ptr[col];
            if (ch == '\t') {
                current_x = next_tab_stop(current_x, tab_width);
                prev = 0;
            } else {
                current_x += ttf_advance_with_kerning(font, prev, ch, scale);
                prev = ch;
            }
        }
    }

    editor->cursor.line = clicked_line;
    editor->cursor.pos_in_line = best_col;
    editor->cursor.pos_in_text = line.start + best_col;
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

void ensure_cursor_visible(Editor *editor) {

    Scroll *scroll = &editor->scroll;
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);
    int lines_on_screen = h / (FONT_HEIGHT * editor->scale);
    int cols_on_screen = w / (FONT_WIDTH * editor->scale);

    if (editor->cursor.line < scroll->y_offset)
        scroll->y_offset = editor->cursor.line;
    if (editor->cursor.line >= scroll->y_offset + lines_on_screen)
        scroll->y_offset = editor->cursor.line - lines_on_screen + 1;

    if (editor->cursor.pos_in_line < scroll->x_offset)
        scroll->x_offset = editor->cursor.pos_in_line;
    if (editor->cursor.pos_in_line >= scroll->x_offset + cols_on_screen)
        scroll->x_offset = editor->cursor.pos_in_line - cols_on_screen + 1;

    clamp_scroll(editor, scroll, editor->scale);
}



void renderCursorScrolled(Editor *editor, float scale, Scroll *scroll) {
    if (editor->cursor.line < 0 || editor->cursor.line >= (int)editor->lines.size)
        return;

    Line line = editor->lines.lines[editor->cursor.line];
    int line_start = line.start;
    int line_length = line.end - line.start;
    int col_end = editor->cursor.pos_in_line;

    // Clamp column to end of line
    if (col_end > line_length)
        col_end = line_length;

    int col_start = scroll->x_offset;
    if (col_start > col_end)
        col_start = col_end;

    // Start cursor position at the same base X offset as your text rendering
    float x = LINE_NUMS_OFFSETP;
    float y = 10 + (editor->cursor.line - scroll->y_offset) * FONT_HEIGHT * scale;

    // Sum real glyph widths from col_start to col_end
    for (int col = col_start; col < col_end; ++col) {
        char c = editor->text.data[line_start + col];

        if (!glyph_cache[(unsigned char)c])
            cache_glyph(c, font);

        Glyph *g = glyph_cache[(unsigned char)c];
        float advance = g ? g->advance * scale : FONT_WIDTH * scale;

        x += advance;
    }

    // Draw the vertical cursor line
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
        glVertex2f(x, y);
        glVertex2f(x, y + FONT_HEIGHT * scale);
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




void render_command_box(Editor *editor, Command_Box *cmd_box){
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);

    float scale_prompt = 0.5f;
    float box_padding = 10.0f;

    float x = 0;
    float y = h;
    float x1 = x + w;
    float y1 = (90 * h) / 100;

    // Background box
    glColor4f(0.251, 0.251, 0.251, 1.0f);
    glBegin(GL_QUADS);
        glVertex2f(x, y1);
        glVertex2f(x1, y1);
        glVertex2f(x1, y);
        glVertex2f(x, y);
    glEnd();

    // Prompt
    float prompt_x = x + box_padding;
    float prompt_y = y - 40;
    renderText(cmd_box->prompt, prompt_x, prompt_y, scale_prompt, WHITE);

    // Command text (user input)
    float cmd_x = (strlen(cmd_box->prompt) - 1) * (FONT_WIDTH * scale_prompt) + 10;
    float cmd_y = prompt_y;
    renderText(cmd_box->command_text.data, cmd_x, cmd_y, scale_prompt, WHITE);

    // Cursor
    float cursor_x = cmd_x;
    for (int i = 0; i < cmd_box->cursor; ++i) {
        char c = cmd_box->command_text.data[i];
        if (!glyph_cache[(unsigned char)c]) {
            cache_glyph(c, font);  // Ensure it's cached
        }
        Glyph* g = glyph_cache[(unsigned char)c];
        if (g) {
            cursor_x += g->advance * scale_prompt;
        } else {
            cursor_x += FONT_WIDTH * scale_prompt;  // Fallback spacing
        }
    }

    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
        glVertex2f(cursor_x, cmd_y);
        glVertex2f(cursor_x, cmd_y + (FONT_HEIGHT * scale_prompt));
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

void editor_center_cursor(Editor *editor) {
    int w, h;
    SDL_GetWindowSize(editor->window, &w, &h);
    int lines_on_screen = h / (FONT_HEIGHT * editor->scale);

    int target_offset = editor->cursor.line - lines_on_screen / 2;
    if (target_offset < 0) target_offset = 0;
    editor->scroll.y_offset = target_offset;

    clamp_scroll(editor, &editor->scroll, editor->scale);
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

    if(editor->lines.size == 0){
        renderText("  1", x, y, scale, vec4f(1.0f, 1.0f, 1.0f, 1.0f));
    }else{
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

    renderText(fb->relative_path.data, 10, 10, fb->scale, WHITE); 

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
            if(fb->search_buffer.size > 0){
        
                float sy = fb->cursor * (FONT_HEIGHT * fb->scale);
        
                glColor4f(0.3f, 0.7f, 1.0f, 0.7f);
        
                glBegin(GL_QUADS);
                    glVertex2f(name_x, y);
                    glVertex2f(name_x + (FONT_WIDTH * fb->scale) * fb->search_buffer.size, y);
                    glVertex2f(name_x + (FONT_WIDTH * fb->scale) * fb->search_buffer.size, y + FONT_HEIGHT * fb->scale);
                    glVertex2f(name_x, y + FONT_HEIGHT * fb->scale);
                glEnd();
        
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



}

void render_keybinds(Settings* settings){

    for(int i = 0; i < 1; i++){

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(30, 30);
            glVertex2f(150, 30);
            glVertex2f(150, 150);
            glVertex2f(30, 150);
        glEnd();


    }

}

void fb_search(File_Browser *fb){       // VERY basic search
    int search_len = fb->search_buffer.size;


    for(int i = 0; i < fb->items.count; i++){

        Strung tmp = strung_init(fb->items.items[i].name);

        if (tmp.size < search_len){
            // do nothing
        } else{
            char* substr = strung_substr(&tmp, 0, search_len);
    
            if(substr == NULL){
                return;
            }
    
            if(strcmp(substr, fb->search_buffer.data) == 0){
                fb->cursor = i;
            }
        }

    }


}

void save_undo_state(Editor* editor) {
    stack_push(&editor->undo_stack, &editor->text, editor->cursor);
    stack_clear(&editor->redo_stack);
}


void undo(Editor* editor) {
    HistoryEntry* entry = stack_pop(&editor->undo_stack);
    if (!entry) return;

    stack_push(&editor->redo_stack, &editor->text, editor->cursor);
    strung_free(&editor->text);
    editor->text = entry->text;
    editor->cursor = entry->cursor;
    free(entry);

    if (editor->cursor.pos_in_text > editor->text.size) {
        editor->cursor.pos_in_text = editor->text.size;
    }

}

void redo(Editor* editor) {
    HistoryEntry* entry = stack_pop(&editor->redo_stack);
    if (!entry) return;

    stack_push(&editor->undo_stack, &editor->text, editor->cursor);
    strung_free(&editor->text);
    editor->text = entry->text;
    editor->cursor = entry->cursor;
    free(entry);

    if (editor->cursor.pos_in_text > editor->text.size) {
        editor->cursor.pos_in_text = editor->text.size;
    }

}

bool keybind_matches(const SDL_Event *event, const Keybind kb)
{
    if (event->type != SDL_KEYDOWN)
        return false;

    if (event->key.keysym.sym != kb.key)
        return false;

    SDL_Keymod mods = event->key.keysym.mod;

    bool ctrl_held  = mods & KMOD_CTRL;
    bool shift_held = mods & KMOD_SHIFT;
    bool alt_held   = mods & KMOD_ALT;

    return (ctrl_held  == kb.ctrl) &&
           (shift_held == kb.shift) &&
           (alt_held   == kb.alt);
}


int main(int argc, char *argv[]) {
    
    
    Editor editor = {
        .cursor = {0}, 
        .file_path = "", 
        .text = strung_init(""),  
        .lines = {0},
        .scale = DEFAULT_EDITOR_SCALE
    };

    char buffer[PATH_MAX];
    if(!(realpath(".", buffer))) fprintf(stderr, "Failed, A lot (at opening init directory)\n");
    File_Browser fb = {.relative_path = strung_init(buffer), .scale = 0.5f, .new_file_path = strung_init(""), .copied_file_contents = strung_init_custom("", 1024)};
    strung_append_char(&fb.relative_path, '/');

    Command_Box cmd_box = {.command_text = strung_init("")};
    
    Settings settings = load_settings(&editor, &cmd_box, &fb);
    // Settings settings = {0};
    // settings.path_to_font = "fonts/MapleMono-Regular.ttf";
    // settings.editor_scale = 0.3f;

    if(TTF_Init() < 0){
        fprintf(stderr, "Failed to initilize TTF\n");
        return 1;
    }

    font = TTF_OpenFont(settings.path_to_font, 48); 

    if(font == NULL){
        fprintf(stderr, "Failed to open font %s, closing application\n", settings.path_to_font);
        exit(1);
    }
    // i dont like the way that '#' look in this font so it WILL be changed later
    // TODO: Have different fonts that are loadable while the application is open


    bool line_switch = false;

    if (argc > 1){
        open_file(&editor, &cmd_box, argv[1]);
        editor_recalculate_lines(&editor);
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    editor.window = SDL_CreateWindow("Basic(?) Code Editor",
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
                    open_file(&editor, &cmd_box,event.drop.file); 
                }
                SDL_free(event.drop.file);                                                               
            }
            else if (event.type == SDL_TEXTINPUT) {
                if(fb.file_browser){ 
                    if(fb.new_file || fb.renaming){
                        strung_append(&fb.new_file_path, event.text.text);
                    } else{
                        strung_append(&fb.search_buffer, event.text.text);
                        fb_search(&fb);
                    }
                } else {
                    if(editor.selection){
                        if (editor.selection_start > editor.selection_end){
                            SEL_SWAP(editor.selection_start, editor.selection_end);
                        }
                        strung_delete_range(&editor.text, editor.selection_start, editor.selection_end);
                        editor.cursor.pos_in_line -= editor.selection_end - editor.selection_start;
                        editor.cursor.pos_in_text -= editor.selection_end - editor.selection_start;
                        editor.selection_start = 0;
                        editor.selection_end = 0;
                        editor.selection = false;
                    }
                    if (!(SDL_GetModState() & KMOD_CTRL) && !cmd_box.in_command) {
                        save_undo_state(&editor);
                        strung_insert_string(&editor.text, event.text.text, editor.cursor.pos_in_text);
                        editor.cursor.pos_in_text += strlen(event.text.text);
                        editor.cursor.pos_in_line += strlen(event.text.text);
                        editor_recalculate_lines(&editor);
                    } else if(cmd_box.in_command){
                        if(!(SDL_GetModState() & KMOD_CTRL)){
                            strung_insert_string(&cmd_box.command_text, event.text.text, cmd_box.cursor);
                            cmd_box.cursor += strlen(event.text.text);
                        }
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                if(cmd_box.in_command){
                    SDL_Keycode key = event.key.keysym.sym;

                    if(key == SDLK_RETURN){
                        cmdbox_command(&editor, &cmd_box, &fb, &settings);
                    }else if (key == SDLK_BACKSPACE){
                        if(cmd_box.cursor > 0){
                            strung_remove_char(&cmd_box.command_text, cmd_box.cursor - 1);
                            cmd_box.cursor--;
                        }
                    }else if (key == SDLK_F3){
                        cmd_box.in_command = !cmd_box.in_command;
                    }else if (key == SDLK_TAB){
                        cmdbox_autocomplete(&cmd_box);
                    }
                }
                else if (fb.file_browser) {
                    SDL_Keycode key = event.key.keysym.sym;

                    if (key == SDLK_F2) {
                        fb.file_browser = false;

                    } else if (key == SDLK_ESCAPE) {
                        running = false;

                    } else if (key == SDLK_UP) {
                        if (fb.cursor > 0 && !fb.renaming) fb.cursor--;

                    } else if (key == SDLK_DOWN) {
                        if (fb.cursor < fb.items.count - 1 && !fb.renaming) fb.cursor++;

                    } else if (key == SDLK_BACKSPACE) {
                        if ((fb.new_file || fb.renaming) && fb.new_file_path.size > 0)
                            strung_remove_char(&fb.new_file_path, fb.new_file_path.size - 1);
                        else
                            strung_remove_char(&fb.search_buffer, fb.search_buffer.size - 1);

                    } else if (key == SDLK_RETURN) {
                        if (fb.new_file){
                            if (fb.new_file_path.size > 0){
                                Strung final = strung_init("");
                                strung_append(&final, fb.relative_path.data);
                                strung_append(&final, fb.new_file_path.data);
                                if (fb.new_dir){
                                    mkdir(final.data, S_IRWXU);
                                } else {
                                    create_new_file(final.data);
                                }
                                read_entire_dir(&fb);
                            }
                            fb.new_file = false;
                            fb.new_dir = false;
                        } else if (fb.renaming){
                            if (fb.new_file_path.size > 0){
                                Strung new_name_path = strung_init("");
                                strung_append(&new_name_path, fb.relative_path.data);
                                strung_append(&new_name_path, fb.new_file_path.data);

                                Strung old_name_path = strung_init("");
                                strung_append(&old_name_path, fb.relative_path.data);
                                strung_append(&old_name_path, fb.items.items[fb.cursor].name);

                                if (rename(old_name_path.data, new_name_path.data) != 0) fprintf(stderr, "Failed to rename\n");
                                read_entire_dir(&fb);
                            }
                            fb.renaming = false;
                        } else {
                            char *selected = fb.items.items[fb.cursor].name;
                            if (fb.items.items[fb.cursor].type == DT_REG) {
                                // Build full path
                                char full_path[PATH_MAX];
                                snprintf(full_path, sizeof(full_path), "%s%s", fb.relative_path.data, selected);
                                char resolved[PATH_MAX];
                                if (!realpath(full_path, resolved)) {
                                    fprintf(stderr, "Failed to get full file path of %s\n", selected);
                                } else {
                                    open_file(&editor, &cmd_box, resolved);
                                    ensure_cursor_visible(&editor);
                                    fb.file_browser = false;
                                }
                            } else if (fb.items.items[fb.cursor].type == DT_DIR) {
                                char *sel = fb.items.items[fb.cursor].name;
                                if (strcmp(sel, ".") == 0) {
                                    // Stay
                                } else if (strcmp(sel, "..") == 0) {
                                    // Go up one directory
                                    size_t len = strlen(fb.relative_path.data);
                                    if (len > 1) {
                                        if (fb.relative_path.data[len - 1] == '/')
                                            fb.relative_path.data[--len] = '\0';
                                        char *slash = strrchr(fb.relative_path.data, '/');
                                        if (slash && slash != fb.relative_path.data) {
                                            *slash = '\0';
                                            len = slash - fb.relative_path.data;
                                        } else {
                                            fb.relative_path.data[0] = '/';
                                            fb.relative_path.data[1] = '\0';
                                            len = 1;
                                        }
                                    }
                                    if (strcmp(fb.relative_path.data, "/") != 0) {
                                        if (fb.relative_path.data[len - 1] != '/') {
                                            fb.relative_path.data[len] = '/';
                                            fb.relative_path.data[len + 1] = '\0';
                                        }
                                    }
                                    fb.relative_path.size = strlen(fb.relative_path.data);
                                } else {
                                    // Go into selected directory
                                    size_t len = strlen(fb.relative_path.data);
                                    if (len > 0 && fb.relative_path.data[len - 1] != '/')
                                        strung_append_char(&fb.relative_path, '/');
                                    strung_append(&fb.relative_path, sel);
                                    strung_append_char(&fb.relative_path, '/');
                                }
                                read_entire_dir(&fb);
                                fb.cursor = 0;
                            }
                        }
                        strung_reset(&fb.search_buffer);

                    } else if (key == SDLK_DELETE) {

                        Strung final_path = strung_init("");
                        strung_append(&final_path, fb.relative_path.data);
                        strung_append(&final_path, fb.items.items[fb.cursor].name);
                        if (!(strcmp(fb.items.items[fb.cursor].name, "..") == 0 || strcmp(fb.items.items[fb.cursor].name, ".") == 0)){
                            if (fb.items.items[fb.cursor].type == DT_DIR){
                                rmdir(final_path.data);
                            } else if (fb.items.items[fb.cursor].type == DT_REG){
                                remove(final_path.data);
                            } else{
                                fprintf(stderr, "Invalid file type to delete\n");
                            }
                        } else{
                            fprintf(stderr, "We do not support deleting current dir or prev dir, for safety reasons\n");
                            fprintf(stderr, "and because i don't know what will happen\n");
                        }
                        read_entire_dir(&fb);

                    } else if (key == SDLK_MINUS) {
                        if (event.key.keysym.mod & KMOD_CTRL) fb.scale -= 0.1;

                    } else if (key == SDLK_EQUALS) {
                        if (event.key.keysym.mod & KMOD_CTRL) fb.scale += 0.1;

                    } else if (key == SDLK_n) {
                        if (event.key.keysym.mod & KMOD_CTRL){
                            if (event.key.keysym.mod & KMOD_SHIFT){
                                fb.new_dir = true;
                            }
                            strung_reset(&fb.new_file_path);
                            fb.new_file = true;
                        }

                    } else if (key == SDLK_r) {
                        if (event.key.keysym.mod & KMOD_CTRL){
                            strung_reset(&fb.new_file_path);
                            strung_append(&fb.new_file_path, fb.items.items[fb.cursor].name);
                            fb.renaming = true;
                        }

                    } else if (key == SDLK_c) {
                        if (event.key.keysym.mod & KMOD_CTRL){
                            fb.copied_file_name = fb.items.items[fb.cursor].name;
                            open_file_into_strung(&fb.copied_file_contents, fb.copied_file_name);
                        }

                    } else if (key == SDLK_v) {
                        if (event.key.keysym.mod & KMOD_CTRL){
                            if (fb.copied_file_name != NULL && fb.copied_file_contents.data != NULL) {
                                Strung final_cpath = strung_init("");
                                strung_append(&final_cpath, fb.relative_path.data);
                                strung_append(&final_cpath, fb.copied_file_name);

                                FILE *f = fopen(final_cpath.data, "w");
                                if (f) {
                                    fprintf(f, "%s", fb.copied_file_contents.data);
                                    fclose(f);
                                    read_entire_dir(&fb);
                                } else {
                                    fprintf(stderr, "Failed to open file for writing: %s\n", final_cpath.data);
                                }
                            } else {
                                fprintf(stderr, "No file copied to paste.\n");
                            }
                        }

                    } else if (key == SDLK_x) {
                        // cut: (no-op here in your original)
                    }

                } else {
                    SDL_Keycode key = event.key.keysym.sym;

                    if (keybind_matches(&event, settings.keybinds.remove_char)) {
                        if(editor.selection){
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
                        } else if (editor.cursor.pos_in_text > 0) {
                            save_undo_state(&editor);

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
                                editor_recalculate_lines(&editor);
                            } else {
                                strung_remove_char(&editor.text, editor.cursor.pos_in_text - 1);
                                editor.cursor.pos_in_text--;
                                if (editor.cursor.pos_in_line > 0) editor.cursor.pos_in_line--;
                            }
                        }

                    } else if (keybind_matches(&event, settings.keybinds.delete_char)) {
                        if (cmd_box.in_command) {
                            if (cmd_box.cursor < cmd_box.command_text.size) {
                                strung_remove_char(&cmd_box.command_text, cmd_box.cursor);
                            }
                        } else if (editor.cursor.pos_in_text < editor.text.size) {
                            save_undo_state(&editor);
                            strung_remove_char(&editor.text, editor.cursor.pos_in_text);
                            editor_recalculate_lines(&editor); // Can optimize a bit if i recalculate only when deleting newline
                        }

                    } else if (keybind_matches(&event, settings.keybinds.newline)) {
                        if(cmd_box.in_command){ 
                            cmdbox_command(&editor, &cmd_box, &fb, &settings);
                        } else{
                            save_undo_state(&editor);
                            strung_insert_char(&editor.text, '\n', editor.cursor.pos_in_text);
                            editor.cursor.pos_in_text++;
                            editor.cursor.line++;
                            editor.cursor.pos_in_line = 0;
                            editor_recalculate_lines(&editor);
                        }

                    } else if (keybind_matches(&event, settings.keybinds.indent)) {
                        save_undo_state(&editor);
                        strung_insert_string(&editor.text, "    ", editor.cursor.pos_in_text);
                        editor.cursor.pos_in_line += 4;
                        editor.cursor.pos_in_text += 4;

                    } else if (keybind_matches(&event, settings.keybinds.start_of_line)) {
                        editor.cursor.pos_in_text = editor.lines.lines[editor.cursor.line].start;
                        editor.cursor.pos_in_line = 0;

                    } else if (keybind_matches(&event, settings.keybinds.end_of_line)) {
                        editor.cursor.pos_in_text = editor.lines.lines[editor.cursor.line].end;
                        editor.cursor.pos_in_line = editor.lines.lines[editor.cursor.line].end - editor.lines.lines[editor.cursor.line].start;

                    } else if (keybind_matches(&event, settings.keybinds.toggle_fb)) {
                        read_entire_dir(&fb);
                        fb.file_browser = true;
                        strung_reset(&fb.search_buffer);

                    } else if (keybind_matches(&event, settings.keybinds.cmdbox)) {
                        cmdbox_reinit(&cmd_box, "Enter Command:", CMD_NONE);
                        cmd_box.in_command = !cmd_box.in_command;

                    } else if (keybind_matches(&event, settings.keybinds.cursor_left)) {
                        if (cmd_box.in_command) {
                            if (cmd_box.cursor > 0) {
                                cmd_box.cursor--;
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
                                    ensure_cursor_visible(&editor);
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
                                    ensure_cursor_visible(&editor);
                                } else if (editor.cursor.pos_in_text > 0) {
                                    editor.cursor.pos_in_text--;
                                    if (editor.cursor.pos_in_line > 0) {
                                        editor.cursor.pos_in_line--;
                                    }
                                    ensure_cursor_visible(&editor);
                                }
                            }
                            if (shift) {
                                editor.selection_end = editor.cursor.pos_in_text;
                            }
                        }

                    } else if (keybind_matches(&event, settings.keybinds.cursor_right)) {
                        if (cmd_box.in_command) {
                            if (cmd_box.cursor < cmd_box.command_text.size) {
                                cmd_box.cursor++;
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
                                ensure_cursor_visible(&editor);
                            } else {
                                if (editor.text.data[editor.cursor.pos_in_text] == '\n') {
                                    editor.cursor.pos_in_line = 0;
                                    editor.cursor.line++;
                                    editor.cursor.pos_in_text++;
                                    ensure_cursor_visible(&editor);
                                } else if (editor.cursor.pos_in_text < editor.text.size) {
                                    editor.cursor.pos_in_text++;
                                    editor.cursor.pos_in_line++;
                                    ensure_cursor_visible(&editor);
                                }
                            }
                            if (shift) {
                                editor.selection_end = editor.cursor.pos_in_text;
                            }
                        }

                    } else if (keybind_matches(&event, settings.keybinds.cursor_up)) {
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
                        ensure_cursor_visible(&editor);

                    } else if (keybind_matches(&event, settings.keybinds.cursor_down)) {

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

                        ensure_cursor_visible(&editor);

                    } else if (keybind_matches(&event, settings.keybinds.quit_app)) {
                        running = 0;

                    } else if (keybind_matches(&event, settings.keybinds.scale_up)) {
                        editor.scale += 0.1;
                    } else if (keybind_matches(&event, settings.keybinds.scale_down)) {
                        editor.scale -= 0.1;
                    } else if(keybind_matches(&event, settings.keybinds.scale_reset)){
                        editor.scale = 0.3f;
                    } else if (keybind_matches(&event, settings.keybinds.select_all)) {
                        editor.selection = true;
                        editor.selection_start = 0;
                        editor.selection_end = editor.text.size;
                    } else if (keybind_matches(&event, settings.keybinds.savef)) {
                        save_file(&editor);
                    } else if (keybind_matches(&event, settings.keybinds.openf)) {
                        cmd_box.in_command = true;
                        cmdbox_reinit(&cmd_box, "Open File:", CMD_OPENF);
                        strung_append(&cmd_box.command_text, fb.relative_path.data);
                        cmd_box.cursor = fb.relative_path.size;
                    } else if (keybind_matches(&event, settings.keybinds.cut)) {
                        save_undo_state(&editor);
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
                    } else if (keybind_matches(&event, settings.keybinds.copy)) {
                        if(editor.selection){
                            if(editor.selection_end < editor.selection_start){
                                SEL_SWAP(editor.selection_start, editor.selection_end)
                            }
                            char* selected = strung_substr(&editor.text, editor.selection_start, editor.selection_end - editor.selection_start);
                            if(SDL_SetClipboardText(selected) < 0){
                                fprintf(stderr, "%s could not be copied to clipboard\n", selected);
                            }
                        } else {}
                    } else if (keybind_matches(&event, settings.keybinds.paste)) {
                        save_undo_state(&editor);
                        char* text = SDL_GetClipboardText();
                        strung_insert_string(&editor.text, text, editor.cursor.pos_in_text);
                        editor.cursor.pos_in_line += strlen(text);
                        editor.cursor.pos_in_text += strlen(text);
                        SDL_free(text);
                        editor_recalculate_lines(&editor);
                    } else if (keybind_matches(&event, settings.keybinds.undo)) {
                        undo(&editor);
                        editor_recalculate_lines(&editor);
                    }else if(keybind_matches(&event, settings.keybinds.redo)){
                        editor_recalculate_lines(&editor);
                        redo(&editor);
                    }else if (keybind_matches(&event, settings.keybinds.scroll_up)) {
                        editor.scroll.y_offset -= 5;
                        editor.cursor.line -= 5;
                        clamp_scroll(&editor, &editor.scroll,editor.scale);

                    } else if (keybind_matches(&event, settings.keybinds.scroll_down)) {
                        editor.scroll.y_offset += 5;
                        editor.cursor.line += 5;
                        clamp_scroll(&editor, &editor.scroll,editor.scale);

                    } else {
                        // default: nothing
                    }
                }

            } else if (event.type == SDL_MOUSEWHEEL) {
                editor.scroll.y_offset -= event.wheel.y * 10;
                clamp_scroll(&editor, &editor.scroll,editor.scale);
            } else if(event.type == SDL_MOUSEBUTTONDOWN){

                if(fb.file_browser){

                }else{
                if (event.button.button == SDL_BUTTON_LEFT && !cmd_box.in_command) {

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
                    int lines_on_screen = win_h / (FONT_HEIGHT *editor.scale);
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
                                    clamp_scroll(&editor, &editor.scroll,editor.scale);

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

                                    renderTextScrolled(&editor, (FONT_WIDTH *editor.scale) * 3 + 10.0f, 10.0f,editor.scale, WHITE);
                                    render_scrollbar(&editor,editor.scale);
                                    render_line_numbers(&editor,editor.scale);

                                    if(cmd_box.in_command){
                                        render_command_box(&editor, &cmd_box);
                                    }else{
                                        renderCursorScrolled(&editor,editor.scale, &editor.scroll);
                                    }

                                    render_selection(&editor,editor.scale, &editor.scroll);

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
                        clamp_scroll(&editor, &editor.scroll,editor.scale);
                        // Don't move cursor or select text if scrollbar was clicked
                        break;
                    }
                }

                
                static Uint32 last_click_time = 0;
                static int last_click_x = 0, last_click_y = 0;
                Uint32 now = SDL_GetTicks();
                int double_click_distance = 5; // pixels

                if (event.button.button == SDL_BUTTON_LEFT && !cmd_box.in_command) {        
                    bool is_double_click = (now - last_click_time < 500 &&
                        abs(event.button.x - last_click_x) < double_click_distance &&
                        abs(event.button.y - last_click_y) < double_click_distance);

                    if (is_double_click) {
                        editor_move_cursor_to_click(&editor, event.button.x, event.button.y,editor.scale, font);

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
                        editor_move_cursor_to_click(&editor, event.button.x, event.button.y,editor.scale, font);
                    }
                    last_click_time = now;
                    last_click_x = event.button.x;
                    last_click_y = event.button.y;
                }
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
        else if(editkeys){
            render_keybinds(&settings);
        }
        else {
            if(editor.file_path[0] == '\0'){
                renderText("Please open a file", 50, 20, 0.5f, WHITE);
            }
            renderTextScrolled(&editor, LINE_NUMS_OFFSET, 10.0f,editor.scale, WHITE);
            render_scrollbar(&editor,editor.scale);
            render_line_numbers(&editor,editor.scale);
            render_selection(&editor,editor.scale, &editor.scroll);
            renderCursorScrolled(&editor,editor.scale, &editor.scroll);
        }
        
        if(cmd_box.in_command){
            char buffer[100];
            render_command_box(&editor, &cmd_box);
        }

        


        SDL_GL_SwapWindow(editor.window);
    }

    strung_free(&editor.text);
    SDL_StopTextInput();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(editor.window);
    SDL_Quit();

    for (int i = 0; i < GLYPH_CACHE_SIZE; ++i) {
    if (glyph_cache[i]) {
        glDeleteTextures(1, &glyph_cache[i]->texture);
        free(glyph_cache[i]);
    }
    }

    update_and_save_settings(settings, editor);

    return 0;
}

