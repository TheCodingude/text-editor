// I get to work on the fun stuff now :D

typedef enum{
    CMD_NONE, // no current mode
    CMD_JMP, // jump to line or end of file 
    CMD_TERM, // run a shell command
    CMD_QUIT, // exiting the program
    CMD_OPENF, // opening a file
    CMD_PASS_SET, // password protection on the file
    CMD_PASS_ENTER,
    CMD_FONT_CHANGE // changes font (obviously)
}Command_type;

typedef struct{
    size_t cursor; // i really don't think this needs a full cursor right? just an index in the text should suffice 
    bool in_command;
    char* prompt; 
    Strung command_text;
    Command_type type;
}Command_Box;


void cmdbox_reinit(Command_Box *cmd_box, char* new_prompt, Command_type type){
    strung_reset(&cmd_box->command_text);
    cmd_box->prompt = new_prompt;
    cmd_box->cursor = 0;
    cmd_box->type = type;
}

#include "filestuff.h"

void cmdbox_parse_command(Editor *editor, Command_Box *cmd_box, File_Browser *fb){

    if(cmd_box->type == CMD_NONE){
        Strung command = strung_copy(&cmd_box->command_text);
        strung_trim(&command);
        int token_count; 
        Strung** tokens = strung_split_by_space(&command, &token_count);


        if(strcmp(tokens[0]->data, "jmp") == 0){ 
            if (token_count < 2){
                cmdbox_reinit(cmd_box, "jmp to: ", CMD_JMP);
            }else if(strcmp(tokens[1]->data, "end") == 0){
                editor->cursor.line = editor->lines.size - 1;
                editor->cursor.pos_in_text = editor->lines.lines[editor->lines.size - 1].start;
                editor->cursor.pos_in_line = 0;
                editor_center_cursor(editor);
            }
            else {
                int line = atoi(tokens[1]->data) - 1;
                if(line >= 0 && line <= editor->lines.size){
                    editor->cursor.line = line;
                    editor->cursor.pos_in_line = 0;
                    editor->cursor.pos_in_text = editor->lines.lines[editor->cursor.line].start;
                    editor_center_cursor(editor);
                    cmd_box->in_command = false;
                }
                else{
                    printf("'%s' is not a valid line\n", tokens[1]->data);
                }
            }
        }
        else if(strcmp(tokens[0]->data, "com") == 0){
            cmdbox_reinit(cmd_box, "Shell Command:", CMD_TERM);
        }
        else if(strcmp(tokens[0]->data, "quit") == 0){
            exit(0);
        }
        else if(strcmp(tokens[0]->data, "open") == 0){
            cmdbox_reinit(cmd_box, "Open File:", CMD_OPENF);
            strung_append(&cmd_box->command_text, fb->relative_path.data);
            cmd_box->cursor = fb->relative_path.size;
        }
        else if(strcmp(tokens[0]->data, "protect") == 0){
            cmdbox_reinit(cmd_box, "Enter Password:", CMD_PASS_SET);
        }
        else if(strcmp(tokens[0]->data, "font") == 0){
            if(token_count < 2){
                cmdbox_reinit(cmd_box, "Choose a font: ", CMD_FONT_CHANGE);
            } else{
                char buf[100];

                snprintf(buf, 100, "./fonts/%s.ttf", tokens[1]->data);

                printf("|%s|", buf);

                font = TTF_OpenFont(buf, 48);   

                memset(glyph_cache, 0, sizeof(Glyph));

                for(char i = 33; i <= 127; i++){
                    cache_glyph(i, font);
                }
                cmdbox_reinit(cmd_box, "Enter Command: ", CMD_NONE);
            }


        }
        else{
            // doing nothing cause running a shell command by default seems kinda annoying 
        }

    }else{
        fprintf(stderr, "UNREACHABLE: cmdbox_parse_command\n");
    }

}

void cmdbox_command(Editor* editor, Command_Box *cmd_box, File_Browser *fb){ // editor is passed so we can manipulate it 

    switch(cmd_box->type){
        case CMD_NONE:
            cmdbox_parse_command(editor, cmd_box, fb);
            break;
        case CMD_JMP:
            int line = atoi(cmd_box->command_text.data) - 1;
            editor->cursor.line = line;
            editor->cursor.pos_in_line = 0;
            editor->cursor.pos_in_text = editor->lines.lines[editor->cursor.line].start;
            editor_center_cursor(editor);
            cmd_box->in_command = false;
            break;
        case CMD_TERM:
            system(cmd_box->command_text.data); // in the future create a subprocess with pipes so we can read stdout and stderr, and provide stdin 
            cmdbox_reinit(cmd_box, cmd_box->prompt, cmd_box->type); // Just in case i switch the wording or whatever in the future
            break;
        case CMD_OPENF:
            char buffer[PATH_MAX];
            open_file(editor, cmd_box, cmd_box->command_text.data);
            if(realpath(cmd_box->command_text.data, buffer)){
                strung_reset(&fb->relative_path);
                strung_append(&fb->relative_path, buffer);
                int idx = strung_search_right(&fb->relative_path, '/');
                if (idx > 0) {
                    strung_delete_range(&fb->relative_path, idx + 1, fb->relative_path.size);
                }

            }else{
                fprintf(stderr, "File no thing\n");
            } 
            cmd_box->in_command = false;
            break;
        case CMD_PASS_SET:
            editor->file_password = cmd_box->command_text.data;
            break;
        case CMD_PASS_ENTER:
            printf("%s", cmd_box->command_text.data);
            break;    
        case CMD_FONT_CHANGE:
            break;
        default:
            printf("Dont forget to break\n");
            break;



    }

}