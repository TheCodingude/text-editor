// TODO: switch from only c keywords to custom keywords based on language files 

typedef enum {
    TOKEN_NONE,
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_SYMBOL
} TokenType;

typedef struct {
    const char* start;
    int length;
    TokenType type;
} Token;



const char* c_keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default",
    "do", "double", "else", "enum", "extern", "float", "for", "goto",
    "if", "inline", "int", "long", "register", "restrict", "return",
    "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof",
    "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
    "_Static_assert", "_Thread_local"
};
const int c_keyword_count = sizeof(c_keywords) / sizeof(c_keywords[0]);

bool is_c_keyword(const char* word, int len) {
    for (int i = 0; i < c_keyword_count; i++) {
        if (strncmp(word, c_keywords[i], len) == 0 && c_keywords[i][len] == '\0') {
            return true;
        }
    }
    return false;
}

const char* c_preprocessors[] = {
    "#define", "#undef", "#include", "#if", "#ifdef", "#ifndef",
    "#else", "#elif", "#endif", "#error", "#pragma", "#line"
};

bool is_preprocessor_directive(const char* line, int len) {
    while (*line == ' ' || *line == '\t') {
        line++; len--;
    }
    for (int i = 0; i < sizeof(c_preprocessors)/sizeof(c_preprocessors[0]); i++) {
        int plen = strlen(c_preprocessors[i]);
        if (len >= plen && strncmp(line, c_preprocessors[i], plen) == 0)
            return true;
    }
    return false;
}

bool is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

Token next_token(const char* src) {
    Token t = {.start = src, .length = 1, .type = TOKEN_SYMBOL};

    if (*src == '\0') {
        t.length = 0;
        t.type = TOKEN_NONE;
        return t;
    }

    // String literal
    if (*src == '"') {
        t.type = TOKEN_STRING;
        const char* s = src + 1;
        while (*s && (*s != '"' || *(s - 1) == '\\')) s++;
        t.length = (s - src) + (*s == '"' ? 1 : 0);
        return t;
    }

    // Single-line comment
    if (src[0] == '/' && src[1] == '/') {
        t.type = TOKEN_COMMENT;
        const char* s = src + 2;
        while (*s && *s != '\n') s++;
        t.length = s - src;
        return t;
    }

    // Multi-line comment
    if (src[0] == '/' && src[1] == '*') {
        t.type = TOKEN_COMMENT;
        const char* s = src + 2;
        while (*s && !(s[0] == '*' && s[1] == '/')) s++;
        t.length = (s - src) + (s[0] == '*' && s[1] == '/' ? 2 : 0);
        return t;
    }

    // Identifier or keyword
    if (isalpha(*src) || *src == '_') {
        const char* s = src;
        while (is_identifier_char(*s)) s++;
        t.length = s - src;
        t.type = is_c_keyword(src, t.length) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
        return t;
    }

    // Default: single char symbol
    t.type = TOKEN_SYMBOL;
    t.length = 1;
    return t;
}

Vec4f token_color(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD: return vec4f(0.7f, 0.3f, 1.0f, 1.0f);  // Purple
        case TOKEN_STRING:  return vec4f(1.0f, 0.8f, 0.2f, 1.0f);  // Yellow
        case TOKEN_COMMENT: return vec4f(0.4f, 1.0f, 0.4f, 1.0f);  // Green
        default:            return WHITE;
    }
}
