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
        printf("Unreachable strung_remove_char");
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




#endif // STRUNG_IMPLEMENTATION