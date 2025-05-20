#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void open_file(Editor *editor, const char* filepath){

    FILE *f = fopen(filepath, "r");
    if(f == NULL){
        return; // TODO: HAVE A POPUP THAT DISPLAYS ERROR  
    } 

    editor->file_path = filepath;
    char buffer[1024];

    while(fgets(buffer, sizeof(buffer), f)){
        strung_append(&editor->text, buffer);
    }

    editor->cursor.pos = editor->text.size;
    
    fclose(f);

}

void save_file(Editor *editor){
    FILE *f = fopen(editor->file_path, "w");

    if (f == NULL){
        return; // TODO: HAVE A POPUP THAT DISPLAYS ERROR
    }

    fprintf(f, editor->text.data); 
    
    fclose(f); 

}
