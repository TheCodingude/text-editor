#include <stdio.h>
#include <string.h>


void open_file(){
    printf("UNIMPLEMENTED\n");
    return;
}

void save_file(Editor *editor){
    FILE *f = fopen(editor->file_path, "w");

    if (f == NULL){
        return; // TODO: HAVE A POPUP THAT DISPLAYS ERROR
    }

    fprintf(f, editor->text.data); // this is fine for now but in the future i would want it to write without having to close the application
    
}
