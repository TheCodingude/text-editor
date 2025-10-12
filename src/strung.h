#ifndef STRUNG_H_
#define STRUNG_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ------------------------------------------------------------
   Strung: simple growable string
   ------------------------------------------------------------ */

typedef struct {
    char* data;
    int   size;      /* number of chars excluding NUL */
    int   capacity;  /* bytes allocated incl. space for NUL */
} Strung;

/* Construction / lifetime */
Strung strung_init(const char* str);                /* from C-string */
Strung strung_init_custom(const char* str, int alloc); /* with min capacity */
Strung strung_make_empty(int reserve_capacity);     /* empty with reserve */
void   strung_free(Strung* str);
void   strung_reset(Strung* str);                   /* keep capacity, set size=0 */

/* Capacity helpers (optional) */
int    strung_reserve(Strung* s, int min_capacity); /* returns 0 ok, -1 oom */
int    strung_shrink_to_fit(Strung* s);             /* returns 0 ok, -1 oom */

/* Append / insert / remove */
void   strung_append(Strung* str, const char* text);
void   strung_append_char(Strung* str, char ch);
void   strung_insert_char(Strung* str, char ch, int position);
void   strung_insert_string(Strung* str, const char* text, int position);
void   strung_remove_char(Strung* str, int position);
void   strung_delete_range(Strung* str, int start, int end); /* [start, end) */

/* Utilities */
void   strung_trim(Strung* s);
char*  strung_substr(const Strung* str, int start, int length); /* malloc'd */
Strung strung_copy(const Strung* src);

/* Tokenization (caller owns results) */
Strung** strung_split_by_space(const Strung *input, int *out_count);
Strung** strung_split_by_multiple_delims(const Strung *input, const char* delims, int *out_count);
void     strung_split_free(Strung** tokens, int count);

/* Search */
int strung_search_left(const Strung* str, char search);  // first index or -1 
int strung_search_right(const Strung* str, char search); // last index or -1 
int strung_starts_with(const Strung* str, char* prefix); // 0 (false), 1 (true) or -1 (error)

#endif /* STRUNG_H_ */

#ifdef STRUNG_IMPLEMENTATION
/* ============================================================
   Implementation
   ============================================================ */

static int strung_ensure_capacity(Strung* s, int needed_min) {
    if (!s) return -1;
    if (needed_min <= s->capacity) return 0;

    int new_cap = s->capacity > 0 ? s->capacity : 16;
    while (new_cap < needed_min) {
        /* grow ~1.5x to reduce realloc churn */
        int grown = new_cap + (new_cap >> 1) + 8;
        if (grown <= new_cap) { /* overflow guard */
            new_cap = needed_min;
            break;
        }
        new_cap = grown;
    }
    char* p = (char*)realloc(s->data, (size_t)new_cap);
    if (!p) return -1;
    s->data = p;
    s->capacity = new_cap;
    return 0;
}

Strung strung_make_empty(int reserve_capacity) {
    Strung s = {0};
    if (reserve_capacity < 1) reserve_capacity = 1;
    s.data = (char*)malloc((size_t)reserve_capacity);
    if (!s.data) {
        s.capacity = 0;
        s.size = 0;
        return s;
    }
    s.size = 0;
    s.capacity = reserve_capacity;
    s.data[0] = '\0';
    return s;
}

Strung strung_init(const char* string) {
    if (!string) string = "";
    int len = (int)strlen(string);
    /* give some headroom */
    int cap = len + 32;
    Strung s = strung_make_empty(cap);
    if (s.data) {
        memcpy(s.data, string, (size_t)len);
        s.size = len;
        s.data[len] = '\0';
    }
    return s;
}

Strung strung_init_custom(const char* string, int alloc) {
    if (!string) string = "";
    int len = (int)strlen(string);
    if (alloc < len + 1) alloc = len + 1;
    Strung s = strung_make_empty(alloc);
    if (s.data) {
        memcpy(s.data, string, (size_t)len);
        s.size = len;
        s.data[len] = '\0';
    }
    return s;
}

int strung_reserve(Strung* s, int min_capacity) {
    if (!s) return -1;
    if (min_capacity < 1) min_capacity = 1;
    return strung_ensure_capacity(s, min_capacity);
}

int strung_shrink_to_fit(Strung* s) {
    if (!s) return -1;
    int need = s->size + 1;
    if (need <= 0) need = 1;
    if (s->capacity == need) return 0;
    char* p = (char*)realloc(s->data, (size_t)need);
    if (!p) return -1;
    s->data = p;
    s->capacity = need;
    return 0;
}

void strung_free(Strung* str) {
    if (!str) return;
    free(str->data);
    str->data = NULL;
    str->size = 0;
    str->capacity = 0;
}

void strung_reset(Strung* str) {
    if (!str || !str->data) return;
    str->size = 0;
    str->data[0] = '\0';
}

void strung_append(Strung* str, const char* text) {
    if (!str) return;
    if (!text) text = "";
    int text_len = (int)strlen(text);
    int need = str->size + text_len + 1;
    if (strung_ensure_capacity(str, need) != 0) {
        fprintf(stderr, "strung_append: out of memory\n");
        return;
    }
    memcpy(str->data + str->size, text, (size_t)text_len);
    str->size += text_len;
    str->data[str->size] = '\0';
}

void strung_append_char(Strung* str, char ch) {
    if (!str) return;
    int need = str->size + 1 + 1;
    if (strung_ensure_capacity(str, need) != 0) {
        fprintf(stderr, "strung_append_char: out of memory\n");
        return;
    }
    str->data[str->size++] = ch;
    str->data[str->size] = '\0';
}

void strung_insert_char(Strung* str, char ch, int position) {
    if (!str) return;
    if (position < 0 || position > str->size) {
        fprintf(stderr, "strung_insert_char: invalid position %d\n", position);
        return;
    }
    int need = str->size + 1 + 1;
    if (strung_ensure_capacity(str, need) != 0) {
        fprintf(stderr, "strung_insert_char: out of memory\n");
        return;
    }
    /* shift including NUL terminator */
    memmove(str->data + position + 1,
            str->data + position,
            (size_t)(str->size - position + 1));
    str->data[position] = ch;
    str->size++;
}

void strung_insert_string(Strung* str, const char* text, int position) {
    if (!str) return;
    if (!text) text = "";
    if (position < 0 || position > str->size) {
        fprintf(stderr, "strung_insert_string: invalid position %d\n", position);
        return;
    }
    int text_len = (int)strlen(text);
    int need = str->size + text_len + 1;
    if (strung_ensure_capacity(str, need) != 0) {
        fprintf(stderr, "strung_insert_string: out of memory\n");
        return;
    }
    memmove(str->data + position + text_len,
            str->data + position,
            (size_t)(str->size - position + 1));
    memcpy(str->data + position, text, (size_t)text_len);
    str->size += text_len;
}

void strung_remove_char(Strung* str, int position) {
    if (!str) return;
    if (position < 0 || position >= str->size) {
        fprintf(stderr, "strung_remove_char: index out of range %d\n", position);
        return;
    }
    memmove(str->data + position,
            str->data + position + 1,
            (size_t)(str->size - position)); /* includes NUL */
    str->size--;
}

void strung_delete_range(Strung* str, int start, int end) {
    if (!str) return;
    if (start < 0) start = 0;
    if (end > str->size) end = str->size;
    if (start >= end) return;
    int num_to_remove = end - start;
    memmove(str->data + start,
            str->data + end,
            (size_t)(str->size - end + 1)); /* includes NUL */
    str->size -= num_to_remove;
}

char* strung_substr(const Strung* str, int start, int length) {
    if (!str || !str->data) return NULL;
    if (start < 0) start = 0;
    if (length < 0) length = 0;
    if (start > str->size) return NULL;
    if (start + length > str->size) length = str->size - start;

    char* buffer = (char*)malloc((size_t)length + 1u);
    if (!buffer) {
        fprintf(stderr, "strung_substr: out of memory\n");
        return NULL;
    }
    memcpy(buffer, str->data + start, (size_t)length);
    buffer[length] = '\0';
    return buffer;
}

Strung strung_copy(const Strung* src) {
    Strung copy = {0};
    if (!src || !src->data) return copy;
    /* preserve capacity to keep amortized behavior similar */
    int cap = src->capacity > 0 ? src->capacity : (src->size + 1);
    copy.data = (char*)malloc((size_t)cap);
    if (!copy.data) return copy;
    memcpy(copy.data, src->data, (size_t)(src->size));
    copy.data[src->size] = '\0';
    copy.size = src->size;
    copy.capacity = cap;
    return copy;
}

void strung_trim(Strung *s) {
    if (!s || !s->data) return;
    int start = 0;
    int end = s->size - 1;

    while (start <= end && isspace((unsigned char)s->data[start])) start++;
    while (end >= start && isspace((unsigned char)s->data[end]))   end--;

    int new_size = (start <= end) ? (end - start + 1) : 0;
    if (new_size > 0 && start > 0) {
        memmove(s->data, s->data + start, (size_t)new_size);
    }
    s->data[new_size] = '\0';
    s->size = new_size;
}

Strung** strung_split_by_space(const Strung *input, int *out_count) {
    if (!input || !out_count) return NULL;
    int capacity = 4;
    int count = 0;
    Strung **tokens = (Strung**)malloc(sizeof(Strung*) * (size_t)capacity);
    if (!tokens) return NULL;

    int i = 0;
    while (i < input->size) {
        while (i < input->size && isspace((unsigned char)input->data[i])) i++;
        if (i >= input->size) break;

        int start = i;
        while (i < input->size && !isspace((unsigned char)input->data[i])) i++;

        int len = i - start;
        if (len > 0) {
            Strung *token = (Strung*)malloc(sizeof(Strung));
            if (!token) break;
            token->size = len;
            token->capacity = len + 1;
            token->data = (char*)malloc((size_t)token->capacity);
            if (!token->data) { free(token); break; }
            memcpy(token->data, input->data + start, (size_t)len);
            token->data[len] = '\0';

            if (count >= capacity) {
                int newcap = capacity * 2;
                Strung **nt = (Strung**)realloc(tokens, sizeof(Strung*) * (size_t)newcap);
                if (!nt) { /* stop early; return what we have */
                    break;
                }
                tokens = nt;
                capacity = newcap;
            }
            tokens[count++] = token;
        }
    }

    *out_count = count;
    return tokens;
}


Strung** strung_split_by_delim(const Strung *input, char delim, int *out_count) {
    if (!input || !out_count) return NULL;
    int capacity = 4;
    int count = 0;
    Strung **tokens = (Strung**)malloc(sizeof(Strung*) * (size_t)capacity);
    if (!tokens) return NULL;

    int i = 0;
    while (i < input->size) {
        while (i < input->size && input->data[i] == delim) i++;
        if (i >= input->size) break;

        int start = i;
        while (i < input->size && input->data[i] != delim) i++;

        int len = i - start;
        if (len > 0) {
            Strung *token = (Strung*)malloc(sizeof(Strung));
            if (!token) break;
            token->size = len;
            token->capacity = len + 1;
            token->data = (char*)malloc((size_t)token->capacity);
            if (!token->data) { free(token); break; }
            memcpy(token->data, input->data + start, (size_t)len);
            token->data[len] = '\0';

            if (count >= capacity) {
                int newcap = capacity * 2;
                Strung **nt = (Strung**)realloc(tokens, sizeof(Strung*) * (size_t)newcap);
                if (!nt) { /* stop early; return what we have */
                    break;
                }
                tokens = nt;
                capacity = newcap;
            }
            tokens[count++] = token;
        }
    }

    *out_count = count;
    return tokens;
}




void strung_split_free(Strung** tokens, int count) {
    if (!tokens) return;
    for (int i = 0; i < count; ++i) {
        if (tokens[i]) {
            free(tokens[i]->data);
            free(tokens[i]);
        }
    }
    free(tokens);
}

int strung_search_left(const Strung* str, char search){
    if (!str || !str->data) return -1;
    for (int i = 0; i < str->size; i++) {
        if (str->data[i] == search) return i;
    }
    return -1;
}

int strung_search_right(const Strung* str, char search){
    if (!str || !str->data) return -1;
    for (int i = str->size - 1; i >= 0; i--) {
        if (str->data[i] == search) return i;
    }
    return -1;
}

int strung_starts_with(const Strung* str, char* prefix){
    int pre_size = strlen(prefix);
    if(pre_size < 1){
        return -1;
    }

    for(int i = 0; i < pre_size; i++){
        if(str->data[i] == prefix[i]){
            continue;
        }
        return 0;
    }
    return 1;
}

Strung** strung_split_by_multiple_delims(const Strung *input, const char* delims, int *out_count) {
    if (!input || !delims || !out_count) return NULL;

    int capacity = 4;
    int count = 0;
    Strung **tokens = (Strung**)malloc(sizeof(Strung*) * capacity);
    if (!tokens) return NULL;

    int i = 0;
    while (i < input->size) {
        /* skip over delimiters */
        while (i < input->size && strchr(delims, input->data[i]) != NULL) {
            i++;
        }
        if (i >= input->size) break;

        int start = i;
        while (i < input->size && strchr(delims, input->data[i]) == NULL) {
            i++;
        }

        int len = i - start;
        if (len > 0) {
            Strung *token = (Strung*)malloc(sizeof(Strung));
            if (!token) break;
            token->size = len;
            token->capacity = len + 1;
            token->data = (char*)malloc(token->capacity);
            if (!token->data) { free(token); break; }

            memcpy(token->data, input->data + start, len);
            token->data[len] = '\0';

            if (count >= capacity) {
                int newcap = capacity * 2;
                Strung **nt = (Strung**)realloc(tokens, sizeof(Strung*) * newcap);
                if (!nt) { /* stop early */ break; }
                tokens = nt;
                capacity = newcap;
            }
            tokens[count++] = token;
        }
    }

    *out_count = count;
    return tokens;
}







#endif /* STRUNG_IMPLEMENTATION */
