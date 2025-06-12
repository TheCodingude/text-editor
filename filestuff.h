#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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


void read_entire_dir(File_Browser *fb){
    
    if(!(fb->relative_path.size > 0)){
        fprintf(stderr, "UNREACHABLE: read_entire_dir\n");
        return;
    }

    fb->items.count = 0;

    DIR *dir = opendir(fb->relative_path.data);
	if (!dir) {
		/* Could not open directory */
		fprintf(stderr, "Cannot open current directory\n");
		exit(1);
	}
    

    FB_item item = {0};
	struct dirent *ent;
    struct stat buf;
	while ((ent = readdir(dir)) != NULL) {
        item.name = ent->d_name;
        item.type = ent->d_type;
        
        stat(ent->d_name, &buf);

        item.data = buf;
        
        FB_items_append(&fb->items, item);
        
    }

    organize_directory(fb);
	closedir(dir);


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


void open_file(Editor *editor, char* filepath){
    editor->file_path = filepath;

    FILE *f = fopen(filepath, "r");
    if(f == NULL){
        return; // TODO: HAVE A POPUP THAT DISPLAYS ERROR  
    } 

    char buffer[1024];

    strung_reset(&editor->text);

    while(fgets(buffer, sizeof(buffer), f)){
        strung_append(&editor->text, buffer);
    }

    editor->cursor.pos_in_text = 0;
    editor->cursor.pos_in_line = 0;
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

    fprintf(f, "%s", editor->text.data); 
    
    fclose(f); 

}
