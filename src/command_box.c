// I get to work on the fun stuff now :D

typedef enum{
    CMD_NONE
}Command_type;

typedef struct{
    Cursor command_cursor;
    bool in_command;
    Strung command_text;
    Command_type type;
}Command_Box;


void cmdbox_parse_command(Editor *editor, Command_Box *cmd_box){

    if(cmd_box->type == CMD_NONE){
         // is this how parsing works? 100%. I just wanna get something working
        if(strcmp(strung_substr(&cmd_box->command_text, 0, 3), "jmp") == 0 && cmd_box->command_text.data[3] == ' '){ 
            editor->cursor.line = atoi(strung_substr(&cmd_box->command_text, 4, 5)) - 1;
            editor->cursor.pos_in_text = editor->lines.lines[editor->cursor.line].start;
            editor->cursor.pos_in_line = 0;
            ensure_cursor_visible(editor); 
        }
        else{
            fprintf(stderr, "Unknown Command\n"); // popup in future
        }

    }else{
        fprintf(stderr, "UNREACHABLE: cmdbox_parse_command\n");
    }

}

void cmdbox_command(Editor* editor, Command_Box *cmd_box){ // editor is passed to we can manipulate it 

    switch(cmd_box->type){
        case CMD_NONE:
            cmdbox_parse_command(editor, cmd_box);
            break;




    }

}