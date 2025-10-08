

// DEFAULT SETTINGS

#define DEFAULT_PATH_TO_FONT "fonts/MapleMono-Regular.ttf"
#define DEFAULT_EDITOR_SCALE 0.3f



typedef struct{
    char* path_to_font;
    float editor_scale;
}Settings;



Settings load_settings(){

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
        
        if (strcmp(tokens[0]->data, "font") == 0){
            settings.path_to_font = tokens[1]->data;
        } 
        else if(strcmp(tokens[0]->data, "ed-scale") == 0){
            settings.editor_scale = atof(tokens[1]->data);
        }
    }



    return settings;

}


void save_settings(const Settings settings){
    
}













