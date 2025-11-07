

// DEFAULT SETTINGS

#define DEFAULT_PATH_TO_FONT "fonts/MapleMono-Regular.ttf"
#define DEFAULT_EDITOR_SCALE 0.3f

#define SETTINGS_COUNT 2

#define STRING_MATCH(str1, str2) strcmp(str1, str2) == 0

#include <SDL2/SDL.h>

Keybinds default_keys;
            
Keybind figure_out_keybind(char* key, char* modifiers){ // 10/10 function name

    Keybind kb = {0};

    kb.key = atoi(key);
    
    int num_of_mods = strlen(modifiers);
    for(int i = 0; i < num_of_mods; i++){
        if(modifiers[i] == 'c'){
            kb.ctrl = true;
        }
        if(modifiers[i] == 's'){
            kb.shift = true;
        }
        if(modifiers[i] == 'a'){
            kb.alt = true;
        }
    }

    return kb;
}


int load_keybinds(Settings* settings, Strung** lines, int line, int lc){

    for(int i = line; i < lc; i++){
        if(strcmp(lines[i]->data, "KEYBINDS-END") == 0) return i;
        


        int token_count = 0;
        Strung** tokens = strung_split_by_delim(lines[i], '|', &token_count);

        if(STRUNG_PNTR_CMP(tokens[0], "remove_char")){
            settings->keybinds.remove_char = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.remove_char.description = "Delete Character Before Cursor";
        }else if(STRUNG_PNTR_CMP(tokens[0], "delete_char")){
            settings->keybinds.delete_char = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.delete_char.description = "Deletes Character After Cursor";
        }else if(STRUNG_PNTR_CMP(tokens[0], "newline")){
            settings->keybinds.newline = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.newline.description = "Inserts a newline";
        }else if(STRUNG_PNTR_CMP(tokens[0], "indent")){
            settings->keybinds.indent = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.indent.description = "Indents the line";
        }else if(STRUNG_PNTR_CMP(tokens[0], "start_of_line")){
            settings->keybinds.start_of_line = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.start_of_line.description = "Moves the cursor to the start of the line";
        }else if(STRUNG_PNTR_CMP(tokens[0], "end_of_line")){
            settings->keybinds.end_of_line = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.end_of_line.description = "Moves the cursor to the end of the line";
        }else if(STRUNG_PNTR_CMP(tokens[0], "toggle_fb")){
            settings->keybinds.toggle_fb = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.toggle_fb.description = "Show the file browser";
        }else if(STRUNG_PNTR_CMP(tokens[0], "cmdbox")){
            settings->keybinds.cmdbox = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.cmdbox.description = "Toggles the command box";
        }else if(STRUNG_PNTR_CMP(tokens[0], "cursor_left")){
            settings->keybinds.cursor_left = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.cursor_left.description = "Move cursor left";
        }else if(STRUNG_PNTR_CMP(tokens[0], "cursor_right")){
            settings->keybinds.cursor_right = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.cursor_right.description = "Move cursor right";
        }else if(STRUNG_PNTR_CMP(tokens[0], "cursor_up")){
            settings->keybinds.cursor_up = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.cursor_up.description = "Move cursor up";
        }else if(STRUNG_PNTR_CMP(tokens[0], "cursor_down")){
            settings->keybinds.cursor_down = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.cursor_down.description = "Move cursor down";
        }else if(STRUNG_PNTR_CMP(tokens[0], "quit_app")){
            settings->keybinds.quit_app = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.quit_app.description = "Close editor";
        }else if(STRUNG_PNTR_CMP(tokens[0], "scale_up")){
            settings->keybinds.scale_up = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.scale_up.description = "Zoom out";
        }else if(STRUNG_PNTR_CMP(tokens[0], "scale_down")){
            settings->keybinds.scale_down = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.scale_down.description = "Zoom in";
        }else if(STRUNG_PNTR_CMP(tokens[0], "select_all")){
            settings->keybinds.select_all = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.select_all.description = "Selects all the text";
        }else if(STRUNG_PNTR_CMP(tokens[0], "save_file")){
            settings->keybinds.savef = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.savef.description = "Saves current file";
        }else if(STRUNG_PNTR_CMP(tokens[0], "open_file")){
            settings->keybinds.openf = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.openf.description = "Opens a file";
        }else if(STRUNG_PNTR_CMP(tokens[0], "cut")){
            settings->keybinds.cut = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.cut.description = "Cuts highlighted text to clipboard";
        }else if(STRUNG_PNTR_CMP(tokens[0], "copy")){
            settings->keybinds.copy = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.copy.description = "Copies highlighted text to clipboard";
        }else if(STRUNG_PNTR_CMP(tokens[0], "paste")){
            settings->keybinds.paste = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.paste.description = "Paste from clipboard";
        }else if(STRUNG_PNTR_CMP(tokens[0], "undo")){
            settings->keybinds.undo = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.undo.description = "Undos previous changes";
        }else if(STRUNG_PNTR_CMP(tokens[0], "redo")){
            settings->keybinds.redo = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.redo.description = "Undos the previous undos";
        }else if(STRUNG_PNTR_CMP(tokens[0], "scroll_up")){
            settings->keybinds.scroll_up = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.scroll_up.description = "Scrolls up x amount of lines";
        }else if(STRUNG_PNTR_CMP(tokens[0], "scroll_down")){
            settings->keybinds.scroll_down = figure_out_keybind(tokens[1]->data, tokens[2]->data);
            settings->keybinds.scroll_down.description = "Scrolls down x amount of lines";
        }



    }

    

}



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
    Strung keybind_data = strung_init("");


    for(int i = 0; i < line_count; i++){
        // god i wish switch statements worked for strings 

        if(strung_starts_with(lines[i], "//")){
            continue;
        }

        int token_count = 0;
        Strung** tokens = strung_split_by_delim(lines[i], '|', &token_count);
        
        if(strcmp(tokens[0]->data, "KEYBINDS-BEGIN") == 0){
            i = load_keybinds(&settings, lines, i+1, line_count);
        }
        
        
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

// sacrifices must be made for ease of use
// i hate this

static void format_key(const Keybind *kb, char *keybuf, size_t kbsz, char *modbuf, size_t msz){
    /* Save the raw keycode number instead of a human readable name */
    snprintf(keybuf, kbsz, "%d", (int)kb->key);

    /* modifiers */
    size_t idx = 0;
    if (kb->ctrl && idx + 1 < msz) modbuf[idx++] = 'c';
    if (kb->shift && idx + 1 < msz) modbuf[idx++] = 's';
    if (kb->alt && idx + 1 < msz) modbuf[idx++] = 'a';

    if (idx == 0) {
        /* write "none" when there are no modifiers; snprintf will safely truncate if msz is small */
        snprintf(modbuf, msz, "none");
    } else {
        modbuf[idx] = '\0';
    }
}


void save_keybinds(const Settings settings){
    // Keybind format: function|keycode|modifers[CSA]   C - ctrl, S - shift, A - alt 
    
    FILE* f = fopen("editor_settings", "a");
    if (!f) {
        printf("Failed to save keybinds\n");
        return;
    }

    char keybuf[32]; 
    char modbuf[4];

    fprintf(f, "\n// Keybinds\nKEYBINDS-BEGIN\n");

    format_key(&settings.keybinds.remove_char, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "remove_char|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.delete_char, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "delete_char|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.newline, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "newline|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.indent, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "indent|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.start_of_line, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "start_of_line|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.end_of_line, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "end_of_line|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.toggle_fb, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "toggle_fb|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.cmdbox, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "cmdbox|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.cursor_left, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "cursor_left|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.cursor_right, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "cursor_right|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.cursor_up, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "cursor_up|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.cursor_down, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "cursor_down|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.quit_app, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "quit_app|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.scale_up, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "scale_up|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.scale_down, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "scale_down|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.select_all, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "select_all|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.savef, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "save_file|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.openf, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "open_file|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.cut, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "cut|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.copy, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "copy|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.paste, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "paste|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.undo, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "undo|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.redo, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "redo|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.scroll_up, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "scroll_up|%s|%s\n", keybuf, modbuf);

    format_key(&settings.keybinds.scroll_down, keybuf, sizeof(keybuf), modbuf, sizeof(modbuf));
    fprintf(f, "scroll_down|%s|%s\n", keybuf, modbuf);

    fprintf(f, "KEYBINDS-END\n");

    fclose(f);
}



void update_and_save_settings(const Settings settings, const Editor editor){
    
    FILE* f = fopen("editor_settings", "w");


    fprintf(f, "// General Settings\n"
               "font|%s\n"
               "ed-scale|%f\n"
               "\n// Previous State\n"
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

    fclose(f);
    save_keybinds(settings);

}













