

// DEFAULT SETTINGS

#define DEFAULT_PATH_TO_FONT "fonts/MapleMono-Regular.ttf"
#define DEFAULT_EDITOR_SCALE 0.3f

#define SETTINGS_COUNT 2





Settings load_settings(Editor* editor, Command_Box* cmd){

    // put all setting defaults in and just change them as needed 
    // this might be a good approach or maybe not i have no idea 
    Settings settings = {
        .path_to_font = DEFAULT_PATH_TO_FONT,
        .editor_scale = DEFAULT_EDITOR_SCALE
    };

    Strung fc = strung_init("");
    
    FILE* f = fopen("editor_settings", "r");


    if(!f){
        printf("Failed to load setting, using default settings\n");
        return settings;
    }

    char buf[1024];
    while (fgets(buf, 1024, f) != NULL){
        strung_append(&fc, buf);
    }
    
    int line_count = 0;



    Strung** lines = strung_split_by_delim(&fc, '\n', &line_count);

    for(int i = 0; i < line_count; i++){
        // god i wish switch statements worked for strings 
        int token_count = 0;
        Strung** tokens = strung_split_by_delim(lines[i], '|', &token_count);
        
        if(token_count < 2) continue;


        if (strcmp(tokens[0]->data, "font") == 0){
            settings.path_to_font = tokens[1]->data;
        } 
        else if(strcmp(tokens[0]->data, "ed-scale") == 0){
            settings.editor_scale = atof(tokens[1]->data);
        }
        else if(strcmp(tokens[0]->data, "last_opened_file") == 0){
            open_file(editor, cmd, tokens[1]->data);
        }
        else if(strcmp(tokens[0]->data, "cur_pos_in_text") == 0){
            editor->cursor.pos_in_text = atoi(tokens[1]->data);
        }
        else if(strcmp(tokens[0]->data, "cur_pos_in_line") == 0){
            editor->cursor.pos_in_line = atoi(tokens[1]->data);
        }
        else if(strcmp(tokens[0]->data, "cur_line") == 0){
            editor->cursor.line = atoi(tokens[1]->data);
        }
        else if(strcmp(tokens[0]->data, "scroll_x") == 0){
            editor->scroll.x_offset = atoi(tokens[1]->data);
        }
        else if(strcmp(tokens[0]->data, "scroll_y") == 0){
            editor->scroll.y_offset = atoi(tokens[1]->data);
        }
    }



    return settings;

}


void update_and_save_settings(const Settings settings, const Editor editor){
    
    FILE* f = fopen("editor_settings", "w");


    fprintf(f, "font|%s\n"
               "ed-scale|%f\n"
               "last_opened_file|%s\n"
               "cur_pos_in_text|%i\n"
               "cur_pos_in_line|%i\n"
               "cur_line|%i\n"
               "scroll_x|%i\n"
               "scroll_y|%i\n"
            ,            
            settings.path_to_font,
            editor.scale,
            editor.file_path,
            editor.cursor.pos_in_text,
            editor.cursor.pos_in_line,
            editor.cursor.line,
            editor.scroll.x_offset,
            editor.scroll.y_offset
            );



}













