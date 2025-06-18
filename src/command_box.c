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


void cmdbox_parse_command(Editor *editor, Command_Box *cmd_box){

    if(cmd_box->type == CMD_NONE){
         // is this how parsing works? 100%. I just wanna get something working
        if(strcmp(strung_substr(&cmd_box->command_text, 0, 3), "jmp") == 0 && cmd_box->command_text.data[3] == ' '){ 
            if(cmd_box->command_text.size > 4){
                int line = atoi(strung_substr(&cmd_box->command_text, 4, 5)) - 1;
                if(line == 0){ // user either didn't put anything or put a non number, or just put 0, which is invalid
                    editor->cursor.line = line;
                    editor->cursor.pos_in_text = editor->lines.lines[editor->cursor.line].start;
                    editor->cursor.pos_in_line = 0;
                    ensure_cursor_visible(editor); 
                } else{ // so we ask for line num
                    strung_reset(&cmd_box->command_text);
                    cmd_box->command_cursor.pos_in_text = 0;
                    cmd_box->command_cursor.pos_in_line = 0;
                    cmd_box->prompt = "jump to: ";
                    cmd_box->type = CMD_JMP;
                }
            } else {
                strung_reset(&cmd_box->command_text);
                cmd_box->prompt = "jump to: ";
                cmd_box->type = CMD_JMP;
            }
        }
        else{
            fprintf(stderr, "Unknown Command\n"); // popup in future
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