#include <stdio.h>
#include <stdlib.h>
#include <string.h>





void read_entire_dir(File_Browser *fb){
    
    if(!fb->current_dir){
        fb->current_dir = ".";
    }

    fb->items.count = 0;

    DIR *dir = opendir(fb->current_dir);
	if (!dir) {
		/* Could not open directory */
		fprintf(stderr, "Cannot open current directory\n");
		exit(1);
	}
    

    FB_item item = {0};
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
        item.name = ent->d_name;
        item.type = ent->d_type;

        FB_items_append(&fb->items, item);
    }

	closedir(dir);


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
