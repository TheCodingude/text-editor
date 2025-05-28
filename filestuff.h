#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void open_file(Editor *editor, char* filepath){
    editor->file_path = filepath;

    FILE *f = fopen(filepath, "r");
    if(f == NULL){
        return; // TODO: HAVE A POPUP THAT DISPLAYS ERROR  
    } 

    char buffer[1024];

    while(fgets(buffer, sizeof(buffer), f)){
        strung_append(&editor->text, buffer);
    }

    // editor->cursor.pos_in_text = editor->text.size;
    
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
