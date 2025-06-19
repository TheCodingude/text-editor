// I get to work on the fun stuff now :D

typedef enum{
    CMD_NONE,
    CMD_JMP
}Command_type;

typedef struct{
    Cursor command_cursor; // i really don't think this needs a full cursor right? just an index in the text should suffice 
    bool in_command;
    char* prompt; 
    Strung command_text;
    Command_type type;
}Command_Box;


void cmdbox_reinit(Command_Box *cmd_box, char* new_prompt, Command_type type){
    strung_reset(&cmd_box->command_text);
    cmd_box->prompt = new_prompt;
    cmd_box->command_cursor.pos_in_text = 0;
    cmd_box->command_cursor.pos_in_line = 0;
    cmd_box->type = type;
}


void cmdbox_parse_command(Editor *editor, Command_Box *cmd_box){

    if(cmd_box->type == CMD_NONE){
        Strung command = strung_copy(&cmd_box->command_text);
        strung_trim(&command);
        int token_count; 
        Strung** tokens = strung_split_by_space(&command, &token_count);


        if(strcmp(tokens[0]->data, "jmp") == 0){ 
            if (token_count < 2){
                cmdbox_reinit(cmd_box, "jmp to: ", CMD_JMP);
            } else {
                int line = atoi(tokens[1]->data) - 1;
                if(line > 0 && line <= editor->lines.size){
                    editor->cursor.line = line;
                    editor->cursor.pos_in_text = 0;
                    ensure_cursor_visible(editor);
                }
                else{
                    printf("'%s' is not a valid line\n", tokens[1]->data);
                }
            }
        }
        else{
            fprintf(stderr, "Unknown Command\n"); // i think it the future if it nothing we will try and run it as a terminal command
        }

    }else{
        fprintf(stderr, "UNREACHABLE: cmdbox_parse_command\n");
    }

}

void cmdbox_command(Editor* editor, Command_Box *cmd_box){ // editor is passed so we can manipulate it 

    switch(cmd_box->type){
        case CMD_NONE:
            cmdbox_parse_command(editor, cmd_box);
            break;
        case CMD_JMP:
            int line = atoi(cmd_box->command_text.data) - 1;
            editor->cursor.line = line;
            editor->cursor.pos_in_text = editor->lines.lines[editor->cursor.line].start;
            editor->cursor.pos_in_line = 0;
            ensure_cursor_visible(editor); 
            break;




    }

}