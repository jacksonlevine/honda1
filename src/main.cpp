#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <functional>
#include <sstream>
#include <fstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define PERLIN_IMP
#include "perlin.h"

#define TEXTUREFACE_IMP
#include "textureface.hpp"

#include <entt/entt.hpp>

enum GameState {
    BEGIN_MENU,
    BEGIN_SETTINGS,
    GAME_ACTIVE,
    GAME_SETTINGS,
};

GameState VISIBLE_STATE = BEGIN_MENU;


//WINDOW
GLFWwindow* WINDOW;
int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;
float FOV = 90;

//MOUSE
double LAST_MOUSE_X = 0;
double LAST_MOUSE_Y = 0;
bool FIRST_MOUSE = false;
bool MOUSE_CAPTURED = false;
float MOUSE_SENSITIVITY = 0.1f;

//CAMERA
double CAMERA_YAW = 0.0f;
double CAMERA_PITCH = 0.0f;
glm::vec3 CAMERA_DIRECTION(0.0f, 0.0f, 1.0f);
glm::vec3 CAMERA_POSITION(0.0f, 0.0f, 0.0f);
glm::vec3 CAMERA_RIGHT(1.0f, 0.0f, 0.0f);
glm::vec3 CAMERA_UP(0.0f, 1.0f, 0.0f);

//GAME-RELATED THINGS?
float GLOBAL_BRIGHTNESS = 1.0;
Perlin p;
entt::registry REGISTRY;

//GENERAL FACTS
glm::vec3 UP(0.0f, 1.0f, 0.0f);
int FACE_WINDING = GL_CCW; //Clockwise if you're looking at the shape.

//TIME
double DELTA_TIME = 0;
double LAST_FRAME = 0;

//SHADERS
GLuint SHADER_1;

//TEXTURES
GLuint TEXTURE_SHEET;
int FONT_SIZE = 20;

//VAO
GLuint VERTEX_ARRAY_OBJECT;

//3D MATH
glm::mat4 MODEL(1.0f);
glm::mat4 PROJECTION(
    glm::perspective(
        glm::radians(FOV), 
        static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT),
        0.01f, 
        1000.0f));
glm::mat4 VIEW(0.0f);
glm::mat4 MVP(0.0f);

//CALLBACKS FOR GL
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int load_text (const char *fp, std::string &out);
int create_window(const char *title);
int create_shader_program(GLuint* prog, const char* vfp, const char* ffp);
int prepare_texture(GLuint *tptr, const char *tpath);
void send_shader_1_uniforms();
void update_time();
void bind_geometry(GLuint vbov, GLuint vbouv, const GLfloat *vertices, const GLfloat *uv, size_t vsize, size_t usize);
void bind_geometry_no_upload(GLuint vbov, GLuint vbouv);
void react_to_input();

void rend_imgui();
void init_imgui();

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

void grid(int xstride, int zstride, float step, glm::vec3 center, std::function<void(float,float,float)> func) {
    for(float i = center.x - xstride/2; i < center.x + xstride/2; i+=step)
    {
        for(float k = center.z - zstride/2; k < center.z + zstride/2; k+=step)
        {
            func(i, k, step);
        }
    }
}

float noise_wrap(float x, float z) {
    float divider = 20.3f;
    float multiplier = 20.0f;
    return static_cast<float>(p.noise(x/divider, z/divider)) * multiplier;
}


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
    init_imgui();

    TextureFace stone(0,0);

    //1 vao and shader for now
    glGenVertexArrays(1, &VERTEX_ARRAY_OBJECT);
    glBindVertexArray(VERTEX_ARRAY_OBJECT);
    glUseProgram(SHADER_1);

    while(!glfwWindowShouldClose(WINDOW)) {
        react_to_input();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        send_shader_1_uniforms();

                    static GLuint vbov = 0;
                    static GLuint vbouv = 0;
                    static glm::vec3 last_cam_pos;

                    std::vector<GLfloat> verts;
                    std::vector<GLfloat> uvs;

                    grid(120, 120, 5, CAMERA_POSITION, [&verts, &uvs, &stone](float i, float k, float step){
                         verts.insert(verts.end(), {
                                i-step/2.0f, noise_wrap(i-step/2.0f, k-step/2.0f) ,k-step/2.0f,
                                i-step/2.0f, noise_wrap(i-step/2.0f, k+step/2.0f) ,k+step/2.0f,
                                i+step/2.0f, noise_wrap(i+step/2.0f, k+step/2.0f) ,k+step/2.0f,

                                i+step/2.0f, noise_wrap(i+step/2.0f, k+step/2.0f) ,k+step/2.0f,
                                i+step/2.0f, noise_wrap(i+step/2.0f, k-step/2.0f) ,k-step/2.0f,
                                i-step/2.0f, noise_wrap(i-step/2.0f, k-step/2.0f) ,k-step/2.0f,
                            });

                            uvs.insert(uvs.end(), {
                                stone.bl.x, stone.bl.y,
                                stone.tl.x, stone.tl.y,
                                stone.tr.x, stone.tr.y,

                                stone.tr.x, stone.tr.y,
                                stone.br.x, stone.br.y,
                                stone.bl.x, stone.bl.y
                            });
                    });





                    if(vbov == 0 || last_cam_pos != CAMERA_POSITION) {
                        last_cam_pos = CAMERA_POSITION;
                        glDeleteBuffers(1, &vbov);
                        glDeleteBuffers(1, &vbouv);
                        glGenBuffers(1, &vbov);
                        glGenBuffers(1, &vbouv);

                        bind_geometry(
                        vbov, vbouv, 
                        verts.data(), uvs.data(), 
                        verts.size()*sizeof(GLfloat),
                        uvs.size()*sizeof(GLfloat));
                    } else {
                        bind_geometry_no_upload(vbov, vbouv);
                    }

                    glDrawArrays(GL_TRIANGLES, 0, verts.size());
                    glBindVertexArray(0);


        rend_imgui();

        glfwSwapBuffers(WINDOW);

        glfwPollEvents();

        update_time();
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}

void react_to_input() {
    if(INPUT_FORWARD) {
        CAMERA_POSITION += CAMERA_DIRECTION * (0.001f + static_cast<float>(DELTA_TIME));
        VIEW = glm::lookAt(CAMERA_POSITION, CAMERA_POSITION + CAMERA_DIRECTION, CAMERA_UP);
        MVP = PROJECTION * VIEW * MODEL;
    }
}

void rend_imgui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin("HiddenWindow", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

    ImGui::Text("Honda v0.0.0");

    ImGui::End();
    ImGui::Begin("Test Window", NULL,  ImGuiWindowFlags_NoBackground);
    ImGui::Button("Hello");

    static char buf[512];
    ImGui::InputTextMultiline("Text Test", buf, 512, ImVec2(300, 100));

    ImGui::SliderFloat("Brightness", &GLOBAL_BRIGHTNESS, 0.0f, 1.0f);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("src/assets/fonts/mimos1.otf", FONT_SIZE);
    io.Fonts->Build();
    io.FontDefault = io.Fonts->Fonts[0];


    ImGui_ImplGlfw_InitForOpenGL(WINDOW, true);

    ImGui_ImplOpenGL3_Init("#version 330");
}

void update_time() {
    double current_frame = glfwGetTime();
    DELTA_TIME = current_frame - LAST_FRAME;
    LAST_FRAME = current_frame;
}

void send_shader_1_uniforms() {
    GLuint mvp_loc = glGetUniformLocation(SHADER_1, "mvp");
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(MVP));

    GLuint cam_pos_loc = glGetUniformLocation(SHADER_1, "camPos");
    glUniform3f(cam_pos_loc, CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);

    GLuint brightness_loc = glGetUniformLocation(SHADER_1, "brightness");
    glUniform1f(brightness_loc, GLOBAL_BRIGHTNESS);
}


void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (MOUSE_CAPTURED) {
        if (FIRST_MOUSE) {
            LAST_MOUSE_X = xpos;
            LAST_MOUSE_Y = ypos;
            FIRST_MOUSE = false;
        }

        double xoffset = xpos - LAST_MOUSE_X;
        double yoffset = LAST_MOUSE_Y - ypos;

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

        CAMERA_DIRECTION.x = static_cast<float>(cos(glm::radians(CAMERA_YAW)) * cos(glm::radians(CAMERA_PITCH)));
        CAMERA_DIRECTION.y = static_cast<float>(sin(glm::radians(CAMERA_PITCH)));
        CAMERA_DIRECTION.z = static_cast<float>(sin(glm::radians(CAMERA_YAW)) * cos(glm::radians(CAMERA_PITCH)));
        CAMERA_DIRECTION = glm::normalize(CAMERA_DIRECTION);

        LAST_MOUSE_X = xpos;
        LAST_MOUSE_Y = ypos;

        CAMERA_RIGHT = glm::normalize(glm::cross(UP, CAMERA_DIRECTION));
        CAMERA_UP = glm::cross(CAMERA_DIRECTION, CAMERA_RIGHT);

        VIEW = glm::lookAt(CAMERA_POSITION, CAMERA_POSITION + CAMERA_DIRECTION, CAMERA_UP);

        MVP = PROJECTION * VIEW * MODEL;
    }
}


void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureMouse) {
        // ImGui is active, so don't handle the mouse event here
        return;
    }
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
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    if(KEY_BINDS.find(key) != KEY_BINDS.end())
    {
        *KEY_BINDS[key] = action;
    }
    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        FIRST_MOUSE = true;
        MOUSE_CAPTURED = false;
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
    glfwSetFramebufferSizeCallback(WINDOW, framebuffer_size_callback);
    glfwSetKeyCallback(WINDOW, key_callback);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Initialize GLEW err" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
    glCullFace(GL_BACK);
    glFrontFace(FACE_WINDING);
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

void bind_geometry(GLuint vbov, GLuint vbouv, const GLfloat *vertices, const GLfloat *uv, size_t vsize, size_t usize)
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

void bind_geometry_no_upload(GLuint vbov, GLuint vbouv)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbov);
    GLint pos_attrib = glGetAttribLocation(SHADER_1, "position");
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbouv);
    GLint uv_attrib = glGetAttribLocation(SHADER_1, "uv");
    glEnableVertexAttribArray(uv_attrib);
    glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    PROJECTION = glm::perspective(
        glm::radians(FOV), 
        static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT),
        0.01f, 
        1000.0f);
    MVP = PROJECTION * VIEW * MODEL;
    GLuint mvp_loc = glGetUniformLocation(SHADER_1, "mvp");
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(MVP));
}