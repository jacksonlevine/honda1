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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

//TEXTURES
GLuint TEXTURE_SHEET;

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
int prepare_texture(GLuint *tptr, const char *tpath);

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
        std::cerr << "Honda 1 window create err" << std::endl;
        return EXIT_FAILURE;
    }
    if(!prepare_texture(&TEXTURE_SHEET, "src/assets/texture.png")) {
        std::cerr << "Couldn't find or load texture." << std::endl;
        return EXIT_FAILURE;
    }
    if(!create_shader_program(
        &SHADER_1, 
        "src/assets/vertex.glsl", 
        "src/assets/fragment.glsl")) {
        std::cerr << "Create SHADER_1 err" << std::endl;
        return EXIT_FAILURE;
    }
    while(!glfwWindowShouldClose(WINDOW)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
        std::cerr << "Missing/could not load vert shade file" << std::endl;
        return -1;
    }
    if(!load_text(ffp, fragText)) {
        std::cerr << "Missing/could not load frag shade file" << std::endl;
        return -1;
    }
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *vertexGLChars = vertexText.c_str();
    const GLchar *fragGLChars = fragText.c_str();
    glShaderSource(vertexShader, 1, &vertexGLChars, NULL);
    glCompileShader(vertexShader);
    glShaderSource(fragmentShader, 1, &fragGLChars, NULL);
    glCompileShader(fragmentShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vert shade comp err: " << infoLog << std::endl;
        return -1;
    }
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Frag shade comp err: " << infoLog << std::endl;
        return -1;
    }
    *prog = glCreateProgram();
    glAttachShader(*prog, vertexShader);
    glAttachShader(*prog, fragmentShader);
    glLinkProgram(*prog);
    glGetProgramiv(*prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(*prog, 512, NULL, infoLog);
        std::cerr << "Shade prog link err: " << infoLog << std::endl;
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
        std::cerr << "GLFW init err" << std::endl;
        return -1;
    }
    WINDOW = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title, NULL, NULL);
    if (!WINDOW) {
        std::cerr << "GLFW window err" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(WINDOW);
    glfwSetInputMode(WINDOW, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(WINDOW, mouse_callback);
    glfwSetMouseButtonCallback(WINDOW, mouse_button_callback);
    glfwSetKeyCallback(WINDOW, key_callback);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Initialize GLEW err" << std::endl;
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

int prepare_texture(GLuint *tptr, const char *tpath)
{
    glGenTextures(1, tptr);
    glBindTexture(GL_TEXTURE_2D, *tptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(tpath, &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        return 1;
    }
    else
    {
        std::cout << "Prepare_texture fail err" << std::endl;
        stbi_image_free(data);
        return -1;
    }
    return -1;
}

void bindGeometry(GLuint vbov, GLuint vboc, GLuint vbouv, const GLfloat *vertices, const GLfloat *colors, const GLfloat *uv, int vsize, int csize, int usize)
{
    GLenum error;
    glBindBuffer(GL_ARRAY_BUFFER, vbov);
    glBufferData(GL_ARRAY_BUFFER, vsize, vertices, GL_STATIC_DRAW);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "Bind geom err (vbov): " << error << std::endl;
    }
    GLint pos_attrib = glGetAttribLocation(SHADER_1, "position");
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, vboc);
    glBufferData(GL_ARRAY_BUFFER, csize, colors, GL_STATIC_DRAW);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "Bind geom err (vboc): " << error << std::endl;
    }
    GLint col_attrib = glGetAttribLocation(SHADER_1, "color");
    glEnableVertexAttribArray(col_attrib);
    glVertexAttribPointer(col_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, vbouv);
    glBufferData(GL_ARRAY_BUFFER, usize, uv, GL_STATIC_DRAW);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "Bind geom err (vbouv): " << error << std::endl;
    }
    GLint uv_attrib = glGetAttribLocation(SHADER_1, "uv");
    glEnableVertexAttribArray(uv_attrib);
    glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void bindGeometryNoUpload(GLuint vbov, GLuint vboc, GLuint vbouv)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbov);
    GLint pos_attrib = glGetAttribLocation(SHADER_1, "position");
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, vboc);
    GLint col_attrib = glGetAttribLocation(SHADER_1, "color");
    glEnableVertexAttribArray(col_attrib);
    glVertexAttribPointer(col_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, vbouv);
    GLint uv_attrib = glGetAttribLocation(SHADER_1, "uv");
    glEnableVertexAttribArray(uv_attrib);
    glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
}
