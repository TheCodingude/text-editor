// all window related functions outside of the main window
// not the os Windows, like the window gui
#define POPUP_WIDTH 330
#define POPUP_HEIGHT 200


bool window_confirmation_popup(char* popup_text, char* window_title){  // this might be where i practice truetype font rendering 
    SDL_Window *popup = SDL_CreateWindow(window_title,
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        POPUP_WIDTH, POPUP_HEIGHT,
                                        /*SDL_WINDOW_POPUP_MENU |*/ SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_OPENGL 
                                        );

    if(!popup){
        fprintf(stderr, "Failed to initialize popup\n");
        return false;
    }    

        SDL_GLContext glContext = SDL_GL_CreateContext(popup);
    if (!glContext) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
        goto defer;
        return false;
    }
    
    bool running = true;
    bool result = false; // confirm(true) or cancel(false)
    while(running){

        SDL_Event event;

        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT){
                running = false;
                return false;
            }else if(event.type == SDL_KEYDOWN){
                switch(event.key.keysym.sym){
                    case SDLK_ESCAPE:
                        goto defer;
                        break;

                }
            }
        }

        int pw, ph;
        SDL_GetWindowSize(popup, &pw, &ph);
        
        glViewport(0, 0, pw, ph);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, pw, ph, 0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        float x1 = 10;
        float y1 = (75 * POPUP_HEIGHT) / 100;
        float x2 = (POPUP_WIDTH / 2) - 10;
        float y2 = POPUP_HEIGHT - 10; 

        glBegin(GL_QUADS);                      // confirm button
            glVertex2f(x1, y2);
            glVertex2f(x2, y2);
            glVertex2f(x2, y1);
            glVertex2f(x1, y1);
        glEnd();

        float x3 = x2 + 20;
        float x4 = POPUP_WIDTH - 10;  

        glBegin(GL_QUADS);      
            glVertex2f(x3, y2);
            glVertex2f(x4, y2);
            glVertex2f(x4, y1);
            glVertex2f(x3, y1);
        glEnd();


        renderText("This is text", 50.0f, 50.0f, 1.5f, WHITE);

        SDL_GL_SwapWindow(popup);
    }

defer:
    running = false; // don't think i need this but it's here anyway
    SDL_DestroyWindow(popup);
    return result;
}

