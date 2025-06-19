#ifndef STRUNG_H_
#define STRUNG_H_

#include <stdlib.h>
#include <string.h>

typedef struct {
    char* data;
    int size;
    int capacity;
} Strung;


Strung strung_init(char* str);
void strung_append(Strung* str, const char* text);
void strung_append_char(Strung* str, char ch);
void strung_insert_char(Strung* str, char ch, int position);
void strung_insert_string(Strung* str, const char* text, int position);
void strung_remove_char(Strung* str, int position);
void strung_free(Strung* str);
void strung_reset(Strung* str);
void strung_trim(Strung* s);
Strung** strung_split_by_space(const Strung *input, int *out_count);


#endif // STRUNG_H_

#ifdef STRUNG_IMPLEMENTATION

Strung strung_init(char* string) {
    Strung str = {0};
    int len = strlen(string);
    int cap = len + 100;

    str.data = (char*)malloc(cap * sizeof(char));
    str.size = len;
    str.capacity = cap;
    if (str.data) {
        memcpy(str.data, string, len);
        str.data[len] = '\0'; // Null-terminate the string
    }
    return str;
}


Strung strung_init_custom(char* string, int alloc){ // custom amount of alloc
    Strung str = {0};
    int len = strlen(string);

    str.data = (char*)malloc(alloc * sizeof(char));
    str.size = len;
    str.capacity = alloc;
    if (str.data) {
        memcpy(str.data, string, len);
        str.data[len] = '\0'; // Null-terminate the string
    }
    return str;
}



void strung_append(Strung* str, const char* text) {
    int text_len = strlen(text);
    if (str->size + text_len + 1 > str->capacity) {
        // Resize the buffer
        int new_capacity = (str->capacity + text_len) * 2;
        char* new_data = (char*)realloc(str->data, new_capacity * sizeof(char));
        if (new_data) {
            str->data = new_data;
            str->capacity = new_capacity;
        } else {
            fprintf(stderr, "Get fucked lol\n");
            return;
        }
    }
    strcpy(str->data + str->size, text);
    str->size += text_len;
}

void strung_append_char(Strung* str, char ch) {
    if (str->size + 1 >= str->capacity) {
        int new_capacity = str->capacity * 2;
        char* new_data = (char*)realloc(str->data, new_capacity * sizeof(char));
        if (new_data) {
            str->data = new_data;
            str->capacity = new_capacity;
        } else {
            // Allocation failed
            printf("oopsie doopsie\n");
            return;
        }
    }
    str->data[str->size] = ch;
    str->size++;
    str->data[str->size] = '\0';
}

void strung_reset(Strung* str) {
    if (str->data) {
        str->data[0] = '\0';
    }
    str->size = 0;
}

void strung_free(Strung* str) {
    free(str->data);
    str->data = NULL;
    str->size = 0;
    str->capacity = 0;
}

void strung_insert_char(Strung* str, char ch, int position) {
    if (position < 0 || position > str->size) {
        fprintf(stderr, "Invalid Position\n");
        return;
    }

    if (str->size + 1 >= str->capacity) {
        // Resize the buffer
        int new_capacity = str->capacity * 2;
        char* new_data = (char*)realloc(str->data, new_capacity * sizeof(char));
        if (new_data) {
            str->data = new_data;
            str->capacity = new_capacity;
        } else {
            // Handle memory allocation failure
            return;
        }
    }

    // Shift characters to the right
    memmove(str->data + position + 1, str->data + position, str->size - position + 1);

    // Insert the character
    str->data[position] = ch;
    str->size++;
}

void strung_insert_string(Strung* str, const char* text, int position) {
    if (position < 0 || position > str->size) {
        // Invalid position
        return;
    }

    int text_len = strlen(text);
    if (str->size + text_len >= str->capacity) {
        // Resize the buffer
        int new_capacity = (str->capacity + text_len) * 2;
        char* new_data = (char*)realloc(str->data, new_capacity * sizeof(char));
        if (new_data) {
            str->data = new_data;
            str->capacity = new_capacity;
        } else {
            fprintf(stderr, "Ur memory sucks lol\n");
            return;
        }
    }

    // Shift characters to the right
    memmove(str->data + position + text_len, str->data + position, str->size - position + 1);

    // Insert the string
    memcpy(str->data + position, text, text_len);
    str->size += text_len;
}

void strung_remove_char(Strung* str, int position) {
    if (position < 0 || position >= str->size) {
        printf("Index out of range strung_remove_char");
        return;
    }
    memmove(str->data + position, str->data + position + 1, str->size - position);
    str->size--;
}

char* strung_substr(const Strung* str, size_t start, size_t length) {
    if (!str || start < 0 || length < 0 || start >= str->size) {
        fprintf(stderr, "cannot take a substring outside of the string\n");
        return NULL;
    }

    char* buffer = (char*)malloc((length + 1) * sizeof(char));
    if (!buffer) {
        fprintf(stderr, "memory allocation failed\n");
        return NULL;
    }

    memcpy(buffer, str->data + start, length);
    buffer[length] = '\0';

    return buffer;
}

void strung_delete_range(Strung* str, int start, int end) {
    if (!str || start < 0 || end > str->size || start >= end) {
        // Invalid range
        return;
    }
    int num_to_remove = end - start;
    memmove(str->data + start, str->data + end, str->size - end + 1);
    str->size -= num_to_remove;
}

Strung strung_copy(const Strung* src) { // creates a copy of source strung
    Strung copy = {0};
    copy.capacity = src->capacity;
    copy.size = src->size;
    copy.data = malloc(copy.capacity);
    memcpy(copy.data, src->data, copy.size);
    copy.data[copy.size] = '\0';
    return copy;
}

void strung_trim(Strung *s) {
    int start = 0;
    int end = s->size - 1;

    while (start <= end && isspace((unsigned char)s->data[start])) {
        start++;
    }

    while (end >= start && isspace((unsigned char)s->data[end])) {
        end--;
    }

    int new_size = (start <= end) ? (end - start + 1) : 0;

    if (start > 0 && new_size > 0) {
        memmove(s->data, s->data + start, new_size);
    }

    s->data[new_size] = '\0';
    s->size = new_size;
}

Strung** strung_split_by_space(const Strung *input, int *out_count) {
    int capacity = 4;
    int count = 0;
    Strung **tokens = malloc(sizeof(Strung*) * capacity);

    int i = 0;
    while (i < input->size) {
        while (i < input->size && isspace((unsigned char)input->data[i])) i++;
        if (i >= input->size) break;

        int start = i;
        while (i < input->size && !isspace((unsigned char)input->data[i])) i++;

        int len = i - start;
        if (len > 0) {
            Strung *token = malloc(sizeof(Strung));
            token->size = len;
            token->capacity = len + 1;
            token->data = malloc(token->capacity);
            memcpy(token->data, &input->data[start], len);
            token->data[len] = '\0';

            if (count >= capacity) {
                capacity *= 2;
                tokens = realloc(tokens, sizeof(Strung*) * capacity);
            }

            tokens[count++] = token;
        }
    }

    *out_count = count;
    return tokens;
}



#endif // STRUNG_IMPLEMENTATION