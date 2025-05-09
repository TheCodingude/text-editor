#include <GLFW/glfw3.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

#include "./filestuff.h"

#define MAX_TEXT_LENGTH 1024


char text[MAX_TEXT_LENGTH] = "";
int text_length = 0;
int cursor_pos = 0;

void render_char(float x, float y, char c) {

    if (c == '\0') return;

    char buffer[99999];
    int num_quads;
    glColor3f(1.0f, 1.0f, 1.0f);
    
    char text[2] = { c, '\0' };
    num_quads = stb_easy_font_print(x, y, text, NULL, buffer, sizeof(buffer));
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}


void drawText(float x, float y, float scale, char* texts) {
    char buffer[99999]; // A buffer for vertices
    int num_quads;

    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    glColor3f(1.0f, 1.0f, 1.0f);

    num_quads = stb_easy_font_print(0, 0, texts, NULL, buffer, sizeof(buffer));

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
    if (text_length < MAX_TEXT_LENGTH - 1) {
        text[cursor_pos++] = (char)codepoint;
        text_length += 1;
        text[text_length] = '\0';
    }
}

bool left = false;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
    // else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    //     printf("Space key pressed!\n");
    else if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS || action == GLFW_REPEAT){
        if (text_length > 0) {
            text[--cursor_pos] = "";
            --text_length;
        }
    }
    else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS){
        if(text_length < MAX_TEXT_LENGTH - 1){  
            text[cursor_pos++] = '\n';
            text_length += 1;
            text[text_length] = '\0';
        }
    }
    else if(key == GLFW_KEY_DELETE && action == GLFW_PRESS){
        text[cursor_pos] = "";
        --text_length;
    }
    else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS){
        --cursor_pos;
    }
    else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS){
        if(cursor_pos < text_length){
            ++cursor_pos;
        }
    }


    
}




int main(int argc, char** argv) {

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    // const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Text Editor", NULL, NULL);

    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-Sync
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 800, 600, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glfwSetCharCallback(window, character_callback);
    glfwSetKeyCallback(window, key_callback);

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();
        glLineWidth(5.0f);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }
        // Set the background color
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if(left == true){
            printf("IM GPNNA IKAFLKNA\n");
        }

        drawText(50.0f, 50.0f, 5.0f, text);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}