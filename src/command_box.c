// I get to work on the fun stuff now :D

typedef enum{
    CMD_NONE, // no current mode
    CMD_JMP, // jump to line or end of file 
    CMD_TERM, // run a shell command
    CMD_QUIT, // exiting the program
    CMD_OPENF, // opening a file
    CMD_PASS_SET, // password protection on the file
    CMD_PASS_ENTER,
    CMD_FONT_CHANGE, // changes font (obviously)
    CMD_KEYBINDS, // Shows keybind editing screen
}Command_type;

typedef struct{
    size_t cursor; 
    bool in_command;
    char* prompt; 
    Strung command_text;
    Command_type type;
}Command_Box;

int fonts_count = 0;
Strung** list_of_fonts;

void cmdbox_reinit(Command_Box *cmd_box, char* new_prompt, Command_type type){
    strung_reset(&cmd_box->command_text);
    cmd_box->prompt = new_prompt;
    cmd_box->cursor = 0;
    cmd_box->type = type;
}

#include "filestuff.h"
// #include "strung.h" // here because vs code highlighting and autocomplete doo doo

void cmdbox_autocomplete(Command_Box* cmd_box){

    if(cmd_box->command_text.size < 1){
        return;
    }

    Strung command = strung_copy(&cmd_box->command_text);
    strung_trim(&command); // idk why i trim and split by space im pretty sure splitting gets rid of all of them anyway but if it works dont touch it
    int token_count; 
    Strung** tokens = strung_split_by_space(&command, &token_count);

    
    if(STRUNG_PNTR_CMP(tokens[0], "font")){

        if(fonts_count < 1){  // open the fonts only on first call, i highly doubt you would change font multiple times in one instance but whatever
            DIR *dir = opendir("fonts");
            if (!dir) {
                fprintf(stderr, "Cannot open fonts directory");
                return;
            }
            Strung tmp = strung_init("");
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
    
                if(ent->d_name[0] == '.') continue;

                Strung fname = strung_init(ent->d_name);
                
                int idx = strung_search_left(&fname, '.');
                strung_delete_range(&fname, idx, fname.size);

                
                

                strung_append(&tmp, fname.data);
                strung_append_char(&tmp, ' ');
    
     
            }
            
            list_of_fonts = strung_split_by_space(&tmp, &fonts_count);
            strung_free(&tmp);
            closedir(dir);
        }

        if(token_count < 2){
            return;
        }


        for(int i = 0; i < fonts_count; i++){
            if(strung_starts_with(list_of_fonts[i], tokens[1]->data)){
                strung_delete_range(&cmd_box->command_text, 5, cmd_box->command_text.size);
                strung_append(&cmd_box->command_text, list_of_fonts[i]->data);
                cmd_box->cursor = cmd_box->command_text.size;

            }
        }
        

        
        

    }
    
}




void cmdbox_parse_command(Editor *editor, Command_Box *cmd_box, File_Browser *fb, Settings* settings){

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

                TTF_Font* fon = TTF_OpenFont(buf, 48);  // use a temp so if it fails i just keep the current one

                if (!fon){ 
                    fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
                    cmdbox_reinit(cmd_box, "Enter Command: ", CMD_NONE);
                    cmd_box->in_command = false;
                    return; 
                } else{
                    font = fon;

                    
                    settings->path_to_font = strung_init(buf).data;  // this dumb shit worked so i dont even care

                    memset(glyph_cache, 0, sizeof(glyph_cache));
    
                    for (int i = 33; i <= 127; ++i) {cache_glyph((char)i, font);}
                }

                
                    

                cmdbox_reinit(cmd_box, "Enter Command: ", CMD_NONE);
                cmd_box->in_command = false;
            }


        }
        else if(strcmp(tokens[0]->data, "keybinds") == 0){
            editkeys = true;
            cmdbox_reinit(cmd_box, "Enter Command: ", CMD_NONE);
            cmd_box->in_command = false;
        }
        else{
            // doing nothing cause running a shell command by default seems kinda annoying 
        }

    }else{
        fprintf(stderr, "UNREACHABLE: cmdbox_parse_command\n");
    }

}

void cmdbox_command(Editor* editor, Command_Box *cmd_box, File_Browser *fb, Settings* settings){ // editor is passed so we can manipulate it 

    switch(cmd_box->type){
        case CMD_NONE:
            cmdbox_parse_command(editor, cmd_box, fb, settings);
            break;
        case CMD_JMP:
            int line = atoi(cmd_box->command_text.data) - 1;
            editor->cursor.line = line;
            editor->cursor.pos_in_line = 0;
            editor->cursor.pos_in_text = editor->lines.lines[editor->cursor.line].start; // readable
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