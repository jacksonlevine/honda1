#pragma once
#include <iostream>
#include <format>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <functional>
#include <sstream>
#include <fstream>

//WINDOW
GLFWwindow* WINDOW;
int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;
float FOV = 90;

//MOUSE
int LAST_MOUSE_X = 0;
int LAST_MOUSE_Y = 0;
bool FIRST_MOUSE = false;
bool MOUSE_CAPTURED = false;
float MOUSE_SENSITIVITY = 0.1f;

//CAMERA
float CAMERA_YAW = 0.0f;
float CAMERA_PITCH = 0.0f;
glm::vec3 CAMERA_DIRECTION(0.0f, 0.0f, 1.0f);

//SHADERS
GLuint SHADER_1;

//3D MATH
glm::mat4 MODEL(1.0f);
glm::mat4 PROJECTION(
    glm::perspective(
        glm::radians(FOV), 
        static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT),
        0.1f, 
        1000.0f));


void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
int load_text (const char *fp, std::string &out);
int create_window(const char *title);
int create_shader_program(GLuint* prog, const char* vfp, const char* ffp);

int INPUT_FORWARD = 0;
int INPUT_LEFT = 0;
int INPUT_RIGHT = 0;
int INPUT_BACK = 0;
int INPUT_JUMP = 0;


std::map<int, int*> KEY_BINDS = {
    {GLFW_KEY_W, &INPUT_FORWARD},
    {GLFW_KEY_A, &INPUT_LEFT},
    {GLFW_KEY_D, &INPUT_RIGHT},
    {GLFW_KEY_S, &INPUT_BACK},
    {GLFW_KEY_SPACE, &INPUT_JUMP},
};


int main() {
    if(!create_window("Honda 1")) {
        std::cerr << "Could not create window." << std::endl;
        return EXIT_FAILURE;
    }
    if(!create_shader_program( 
        &SHADER_1, 
        "src/assets/vertex.glsl", 
        "src/assets/fragment.glsl")) {
        std::cerr << "Could not create shader program." << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}




void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (MOUSE_CAPTURED) {
        if (FIRST_MOUSE) {
            LAST_MOUSE_X = xpos;
            LAST_MOUSE_Y = ypos;
            FIRST_MOUSE = false;
        }

        float xoffset = xpos - LAST_MOUSE_X;
        float yoffset = LAST_MOUSE_Y - ypos;

        xoffset *= MOUSE_SENSITIVITY;
        yoffset *= MOUSE_SENSITIVITY;

        CAMERA_YAW += xoffset;
        CAMERA_PITCH += yoffset;

        if (CAMERA_PITCH > 89.0f) {
            CAMERA_PITCH = 89.0f;
        }
        if (CAMERA_PITCH < -89.0f) {
            CAMERA_PITCH = -89.0f;
        }

        CAMERA_DIRECTION.x = cos(glm::radians(CAMERA_YAW)) * cos(glm::radians(CAMERA_PITCH));
        CAMERA_DIRECTION.y = sin(glm::radians(CAMERA_PITCH));
        CAMERA_DIRECTION.z = sin(glm::radians(CAMERA_YAW)) * cos(glm::radians(CAMERA_PITCH));
        CAMERA_DIRECTION = glm::normalize(CAMERA_DIRECTION);

        LAST_MOUSE_X = xpos;
        LAST_MOUSE_Y = ypos;
    }
}


void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (!MOUSE_CAPTURED)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            FIRST_MOUSE = true;
            MOUSE_CAPTURED = true;
        }
        else
        {
            // Game::instance->onLeftClick();
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Game::instance->onRightClick();
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if(KEY_BINDS.find(key) != KEY_BINDS.end())
    {
        *KEY_BINDS[key] = action;
    }
}

int create_shader_program(GLuint* prog, const char* vfp, const char* ffp) {
    std::string vertexText;
    std::string fragText;
    if(!load_text(vfp, vertexText)) {
        std::cerr << "Could not load vertex shader file!" << std::endl;
        return -1;
    }
    if(!load_text(ffp, fragText)) {
        std::cerr << "Could not load fragment shader file!" << std::endl;
        return -1;
    }
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *vertexGLChars = vertexText.c_str();
    const GLchar *fragGLChars = vertexText.c_str();
    glShaderSource(vertexShader, 1, &vertexGLChars, NULL);
    glCompileShader(vertexShader);
    glShaderSource(fragmentShader, 1, &fragGLChars, NULL);
    glCompileShader(fragmentShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation error: " << infoLog << std::endl;
        return -1;
    }
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation error: " << infoLog << std::endl;
        return -1;
    }
    *prog = glCreateProgram();
    glAttachShader(*prog, vertexShader);
    glAttachShader(*prog, fragmentShader);
    glLinkProgram(*prog);
    glGetProgramiv(*prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(*prog, 512, NULL, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
        return -1;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return 1;
}

int load_text (const char *fp, std::string &out) {
    std::ifstream text(fp);
    if(!text.is_open()) { return -1; }
    std::stringstream textstrm;
    textstrm << text.rdbuf();
    text.close();
    out = textstrm.str();
    return 1;
}

int create_window(const char *title) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    WINDOW = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title, NULL, NULL);
    if (!WINDOW) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(WINDOW);
    glfwSetInputMode(WINDOW, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(WINDOW, mouse_callback);
    glfwSetMouseButtonCallback(WINDOW, mouse_button_callback);
    glfwSetKeyCallback(WINDOW, key_callback);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    glDepthFunc(GL_LESS);
    return 1;
}