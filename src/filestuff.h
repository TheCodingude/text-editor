#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

void organize_directory(File_Browser *fb){
    if (fb->items.count < 2) return;

    for (size_t i = 0; i < fb->items.count - 1; ++i) {
        for (size_t j = i + 1; j < fb->items.count; ++j) {
            // Put directories (folders) on top
            int i_is_dir = (fb->items.items[i].type == DT_DIR);
            int j_is_dir = (fb->items.items[j].type == DT_DIR);

            if ((!i_is_dir && j_is_dir) ||
                ((i_is_dir == j_is_dir) && strcmp(fb->items.items[i].name, fb->items.items[j].name) > 0)) {
                FB_item temp = fb->items.items[i];
                fb->items.items[i] = fb->items.items[j];
                fb->items.items[j] = temp;
            }
        }
    }
}


void read_entire_dir(File_Browser *fb) {
    if (fb->relative_path.size == 0) {
        fprintf(stderr, "UNREACHABLE: read_entire_dir\n");
        return;
    }

    fb->items.count = 0;

    DIR *dir = opendir(fb->relative_path.data);
    if (!dir) {
        fprintf(stderr, "Cannot open directory: %s\n", fb->relative_path.data);
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {

        FB_item item = {0};
        item.name = strdup(ent->d_name);  // copy so it's not overwritten

        // Build full path to get stat data
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s%s", fb->relative_path.data, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            item.data = st;
            item.type = S_ISDIR(st.st_mode) ? DT_DIR : DT_REG;
        } else {
            item.type = DT_UNKNOWN;
        }

        FB_items_append(&fb->items, item);
    }

    closedir(dir);
    organize_directory(fb);
}


void print_hash(const unsigned char *hash, unsigned int len) {
    for (unsigned int i = 0; i < len; i++)
        printf("%02x", hash[i]);
    printf("\n");
}

void hash_password(const char* password, unsigned char* hash, unsigned int* hash_len){


    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return;

    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, password, strlen(password));
    EVP_DigestFinal_ex(ctx, hash, hash_len);

    EVP_MD_CTX_free(ctx);

}


void file_encrypt(Editor *editor, FILE *f){
    if (!editor || !editor->file_password || !editor->text.data) return;

    // Use a salt for PBKDF2 (can be stored in file header for decryption)
    unsigned char salt[16] = {0};
    if (fwrite(salt, 1, sizeof(salt), f) != sizeof(salt)) return; // Write salt to file
    fprintf(f, "\n");
    // Derive key and IV from password using PBKDF2 
    unsigned char key_iv[48]; // 32 bytes for key, 16 bytes for IV
    unsigned char *key = key_iv;      // first 32 bytes
    unsigned char *iv = key_iv + 32;  // next 16 bytes
    if (PKCS5_PBKDF2_HMAC(editor->file_password, strlen(editor->file_password),
                          salt, sizeof(salt), 100000, EVP_sha256(), sizeof(key_iv), key_iv) != 1) {
        return;
    }
    // key and iv are now set

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    int outlen1 = editor->text.size + AES_BLOCK_SIZE;
    unsigned char *ciphertext = malloc(outlen1);
    if (!ciphertext) {
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    int len = 0, ciphertext_len = 0;
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char*)editor->text.data, editor->text.size) != 1) {
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    ciphertext_len += len;

    fwrite(ciphertext, 1, ciphertext_len, f);

    free(ciphertext);
    EVP_CIPHER_CTX_free(ctx);

}




void create_new_file(const char* fp){
    FILE* f = fopen(fp,"w");

    fclose(f); // this might be it?
}

void move_file_to_fb(File_Browser *fb, char* filepath){
    Strung contents = strung_init("");

    char buffer[1024];

    FILE *f = fopen(filepath, "r");
    if(f == NULL){
        fprintf(stderr, "Failed to move %s\n", filepath); // TODO: have popup
        strung_free(&contents);
        return;
    }

    while(fgets(buffer, sizeof(buffer), f)){
        strung_append(&contents, buffer);
    }

    // get filepath
    const char *filename = strrchr(filepath, '/');
    if (filename) {
        filename++; // skip the '/'
    } else {
        filename = filepath;
    }

    Strung new_path = strung_init(fb->relative_path.data);
    strung_append_char(&new_path, '/');
    strung_append(&new_path, filename);

    FILE *cf = fopen(new_path.data, "w");
    if (cf == NULL) {
        fprintf(stderr, "Failed to create %s\n", new_path.data);
        strung_free(&contents);
        strung_free(&new_path);
        fclose(f);
        return;
    }

    fprintf(cf, "%s", contents.data);

    strung_free(&contents);
    strung_free(&new_path);

    fclose(f);
    fclose(cf);

    read_entire_dir(fb);

}

void open_file_into_strung(Strung *buff, char* file_path){

    FILE *f = fopen(file_path, "r");
    if(f == NULL){
        return; // TODO: HAVE A POPUP THAT DISPLAYS ERROR  
    } 

    strung_reset(buff);

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while((read = getline(&line, &len, f)) != -1){
        strung_append(buff, line);
    }

    if (line != NULL) {
        free(line);
    }
    fclose(f);
}


void open_file(Editor *editor, Command_Box *cmd_box, char* filepath){
    editor->file_path = filepath;

    FILE *f = fopen(filepath, "r");
    if(f == NULL){
        return; // TODO: HAVE A POPUP THAT DISPLAYS ERROR  
    } 

    Strung buf = strung_init("");

    char buffer[1024];

    strung_reset(&editor->text);

    while(fgets(buffer, sizeof(buffer), f)){
        strung_append(&buf, buffer);
    }

    if(buf.size > 5){
        if(strcmp(strung_substr(&buf, 0, 5), "PASS ") == 0){
            cmdbox_reinit(cmd_box, "Enter Password:", CMD_PASS_ENTER);
            cmd_box->in_command = true;
            return;
        }else{
            strung_append(&editor->text, buf.data);
        }
    }else {
        strung_append(&editor->text, buf.data);
    }



    editor->cursor.pos_in_text = 0;
    editor->cursor.pos_in_line = 0;
    editor->cursor.line = 0;
    editor->scroll.x_offset = 0;
    editor->scroll.y_offset = 0;

    editor_recalculate_lines(editor);
    
    fclose(f);

}

void save_file(Editor *editor){
    FILE *f = fopen(editor->file_path, "w");

    if (f == NULL){
        return; // TODO: HAVE A POPUP THAT DISPLAYS ERROR
    }

    if(editor->file_password){
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len;

        hash_password(editor->file_password, hash, &hash_len);
        fprintf(f, "PASS ");
        for (int i = 0; i < hash_len; ++i) {
            fprintf(f, "%02x", hash[i]);
        }
        fprintf(f, "\n");
        file_encrypt(editor, f);
    }else{
        fprintf(f, "%s", editor->text.data); 
    }

    
    fclose(f); 

}
