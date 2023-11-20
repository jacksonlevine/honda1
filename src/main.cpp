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
#include <thread>
#include <mutex>


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
float SPEED_MULTIPLIER = 2.0f;

//GENERAL FACTS
glm::vec3 UP(0.0f, 1.0f, 0.0f);
int FACE_WINDING = GL_CW; //Counter clockwise if you're looking at the shape.

//TIME
double DELTA_TIME = 0;
double LAST_FRAME = 0;

//SHADERS
GLuint SHADER_FAR;
GLuint SHADER_STANDARD;
GLuint SHADER_BILLBOARD;

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
        5000.0f));
glm::mat4 VIEW(0.0f);
glm::mat4 MVP(0.0f);
glm::mat4 MODELVIEW(0.0f);

//CALLBACKS FOR GL
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int load_text (const char *fp, std::string &out);
int create_window(const char *title);
int create_shader_program(GLuint* prog, const char* vfp, const char* ffp);
int create_shader_program_with_geometry_shader(GLuint* prog, const char* vfp, const char* ffp, const char* gfilepath);
int prepare_texture(GLuint *tptr, const char *tpath);
void send_SHADER_FAR_uniforms();
void send_SHADER_STANDARD_uniforms();
void send_SHADER_BILLBOARD_uniforms();

void update_time();
void bind_geometry(GLuint vbov, GLuint vbouv, const GLfloat *vertices, const GLfloat *uv, size_t vsize, size_t usize, GLuint shader);
void bind_geometry_no_upload(GLuint vbov, GLuint vbouv, GLuint shader);
void react_to_input();
float noise_wrap(float x, float z);
void grid(int xstride, int zstride, float step, glm::vec3 center, std::function<void(float,float,float)> func);

void rend_imgui();
void init_imgui();

int INPUT_FORWARD = 0;
int INPUT_LEFT = 0;
int INPUT_RIGHT = 0;
int INPUT_BACK = 0;
int INPUT_JUMP = 0;
int INPUT_SHIFT = 0;


std::map<int, int*> KEY_BINDS = {
    {GLFW_KEY_W, &INPUT_FORWARD},
    {GLFW_KEY_A, &INPUT_LEFT},
    {GLFW_KEY_D, &INPUT_RIGHT},
    {GLFW_KEY_S, &INPUT_BACK},
    {GLFW_KEY_SPACE, &INPUT_JUMP},
    {GLFW_KEY_LEFT_SHIFT, &INPUT_SHIFT}
};


struct MeshComponent {
public:
    GLuint vbov;
    GLuint vbouv;
    int length;
    MeshComponent();
};

MeshComponent::MeshComponent() : length(0) {
    glGenBuffers(1, &this->vbov);
    GLenum error1 = glGetError();
    if (error1 != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glGenBuffers vbov: " << error1 << std::endl;
    }

    glGenBuffers(1, &this->vbouv);
    GLenum error3 = glGetError();
    if (error3 != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glGenBuffers vbouv: " << error3 << std::endl;
    }
}


#define BLOCKCHUNKWIDTH 16
#define BLOCKCHUNKHEIGHT 64


class BlockChunk {
public:
    entt::entity me;
    int nuggo_pool_index;
    glm::ivec2 position;
    void rebuild();
    void move_to(glm::ivec2 newpos);
    BlockChunk();
};

std::vector<BlockChunk> CHUNKS;

class Nuggo {
public:
    std::vector<GLfloat> verts;
    std::vector<GLfloat> uvs;
    entt::entity me;
};

std::vector<int> chunks_to_rebuild;
std::mutex CTR_MUTEX;


std::vector<Nuggo> NUGGO_POOL;

BlockChunk::BlockChunk() {
    this->me = REGISTRY.create();
    this->nuggo_pool_index = NUGGO_POOL.size();
    Nuggo myNuggo;
    myNuggo.me = me;
    NUGGO_POOL.push_back(myNuggo);
}

void BlockChunk::move_to(glm::ivec2 newpos) {
    this->position = newpos;
}

enum CubeFace {
    LEFT = 0, RIGHT, FORWARD, BACK, TOP, BOTTOM
};

enum BlockTypes {
    STONE, GRASS
};

TextureFace BlockTextures[2] = {
    TextureFace(0,0),
    TextureFace(1,0)
};

bool has_block(int x, int y, int z) {
    if(noise_wrap(x,z) >= y) {
        return true;
    }
    return false;
}

bool has_block(glm::ivec3 &i) {
    if(noise_wrap(i.x, i.z) >= i.y) {
        return true;
    }
    return false;
}


void BlockChunk::rebuild() {

    std::vector<GLfloat> verts;
    std::vector<GLfloat> uvs;

    float pushup = 0.0f;
    grid(BLOCKCHUNKWIDTH, BLOCKCHUNKWIDTH, 1.0f, glm::vec3(position.x*BLOCKCHUNKWIDTH, 0, position.y*BLOCKCHUNKWIDTH), [&verts, &uvs, &pushup](float i, float k, float step){
        verts.insert(verts.end(), {

        i-step/2.0f, noise_wrap(i-step/2.0f, k-step/2.0f)+pushup, k-step/2.0f,
        i+step/2.0f, noise_wrap(i+step/2.0f, k-step/2.0f)+pushup, k-step/2.0f,
        i+step/2.0f, noise_wrap(i+step/2.0f, k+step/2.0f)+pushup, k+step/2.0f,
        i+step/2.0f, noise_wrap(i+step/2.0f, k+step/2.0f)+pushup, k+step/2.0f,
        i-step/2.0f, noise_wrap(i-step/2.0f, k+step/2.0f)+pushup, k+step/2.0f,
        i-step/2.0f, noise_wrap(i-step/2.0f, k-step/2.0f)+pushup, k-step/2.0f,
            
        });

        TextureFace &face = noise_wrap(i,k) > 6 ? BlockTextures[BlockTypes::STONE] : BlockTextures[BlockTypes::GRASS];

        uvs.insert(uvs.end(), {
            face.bl.x, face.bl.y,
            face.tl.x, face.tl.y,
            face.tr.x, face.tr.y,

            face.tr.x, face.tr.y,
            face.br.x, face.br.y,
            face.bl.x, face.bl.y
        });
    });



    bool found = false;
    for(auto c : chunks_to_rebuild) {
        if(NUGGO_POOL[c].me == this->me) {
            found = true;
        }
    }
    if(!found) {
        NUGGO_POOL[this->nuggo_pool_index].verts = verts;
        NUGGO_POOL[this->nuggo_pool_index].uvs = uvs;
        chunks_to_rebuild.push_back(nuggo_pool_index);
    }



}


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
    float divider = 50.3f;
    float multiplier = 30.0f;
    
    float biggermult = 3.0f;

    float onoise = static_cast<float>(p.noise(x/(divider*biggermult), z/(divider*biggermult))) * (multiplier*biggermult);
    return (static_cast<float>(p.noise(x/divider, z/divider)) * multiplier) + onoise;
}

#define LOAD_AFTER_DISTANCE 1

#define CHUNK_LOAD_RADIUS 4




void chunk_thread() {
    glm::ivec3 last_cam_pos_divided;
    while(!glfwWindowShouldClose(WINDOW)) {
        glm::ivec3 curr_cam_divided = glm::ivec3(CAMERA_POSITION)/5;
        if(curr_cam_divided != last_cam_pos_divided) {
            last_cam_pos_divided = curr_cam_divided;
            int index = 0;
            CTR_MUTEX.lock();
            chunks_to_rebuild.clear();
            for(int i = -CHUNK_LOAD_RADIUS; i < CHUNK_LOAD_RADIUS; ++i) {
                for(int k = -CHUNK_LOAD_RADIUS; k < CHUNK_LOAD_RADIUS; ++k) {
                    glm::ivec3 worldcampos(CAMERA_POSITION/static_cast<float>(BLOCKCHUNKWIDTH));
                    glm::ivec2 newchunkpos(worldcampos.x+i, worldcampos.z+k);
                    CHUNKS[index].move_to(newchunkpos);
                    CHUNKS[index].rebuild();
                    index++;
                }
            }
            CTR_MUTEX.unlock();
        }
    }
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
        &SHADER_FAR, 
        "src/assets/farshader/vertex.glsl", 
        "src/assets/farshader/fragment.glsl")) {
        std::cerr << "Create SHADER_FAR err" << std::endl;
        return EXIT_FAILURE;
    }
    if(!create_shader_program(
        &SHADER_STANDARD, 
        "src/assets/shader/vertex.glsl", 
        "src/assets/shader/fragment.glsl")) {
        std::cerr << "Create SHADER_STANDARD err" << std::endl;
        return EXIT_FAILURE;
    }
    if(!create_shader_program(
        &SHADER_BILLBOARD, 
        "src/assets/billboardshader/vertex.glsl", 
        "src/assets/billboardshader/fragment.glsl")) {
        std::cerr << "Create SHADER_BILLBOARD err" << std::endl;
        return EXIT_FAILURE;
    }
    init_imgui();


    //1 vao and shader for now
    glGenVertexArrays(1, &VERTEX_ARRAY_OBJECT);
    glBindVertexArray(VERTEX_ARRAY_OBJECT);

    GLuint VAO2;

    glGenVertexArrays(1, &VAO2);



    //SPAWN CHUNKS

    for(int i = -CHUNK_LOAD_RADIUS; i < CHUNK_LOAD_RADIUS; ++i) {
        for(int k = -CHUNK_LOAD_RADIUS; k < CHUNK_LOAD_RADIUS; ++k) {
            BlockChunk b;
            b.move_to(glm::ivec2(i, k));
            b.rebuild();
            CHUNKS.push_back(b);
        }
    }

    //START THREAD TO REBUILD THEM

    std::thread ct(chunk_thread);
    ct.detach();


    

    auto meshes_view = REGISTRY.view<MeshComponent>();

                    

    while(!glfwWindowShouldClose(WINDOW)) {
        react_to_input();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



                    glBindVertexArray(VERTEX_ARRAY_OBJECT);
                    glUseProgram(SHADER_STANDARD);

                    send_SHADER_STANDARD_uniforms();


                    if(chunks_to_rebuild.size() > 0) {
                        if (CTR_MUTEX.try_lock()) {
                        Nuggo &n = NUGGO_POOL[chunks_to_rebuild.back()];
                            if (!REGISTRY.all_of<MeshComponent>(n.me))
                            {
                                //std::cout << "You dont have a mesh component" << std::endl;
                                MeshComponent m;
                                m.length = n.verts.size();
                                bind_geometry(
                                    m.vbov,
                                    m.vbouv,
                                    n.verts.data(),
                                    n.uvs.data(),
                                    n.verts.size() * sizeof(GLfloat),
                                    n.uvs.size() * sizeof(GLfloat),
                                    SHADER_STANDARD
                                );
                                REGISTRY.emplace<MeshComponent>(n.me, m);
                            }
                            else {
                                //std::cout << "You have a mesh component" << std::endl;
                                MeshComponent& m = REGISTRY.get<MeshComponent>(n.me);

                                glDeleteBuffers(1, &m.vbov);
                                glDeleteBuffers(1, &m.vbouv);
                                glGenBuffers(1, &m.vbov);
                                glGenBuffers(1, &m.vbouv);

                                m.length = n.verts.size();
                                bind_geometry(
                                    m.vbov,
                                    m.vbouv,
                                    n.verts.data(),
                                    n.uvs.data(),
                                    n.verts.size() * sizeof(GLfloat),
                                    n.uvs.size() * sizeof(GLfloat),
                                    SHADER_STANDARD
                                );
                            }
                        chunks_to_rebuild.pop_back();
                        CTR_MUTEX.unlock();
                        }
                    }






                    for (const entt::entity entity : meshes_view)
                    {
                        MeshComponent& m = REGISTRY.get<MeshComponent>(entity);
                        bind_geometry_no_upload(
                            m.vbov,
                            m.vbouv,
                            SHADER_STANDARD);

                        glDrawArrays(GL_TRIANGLES, 0, m.length);

                    }

                    glUseProgram(SHADER_FAR);

        send_SHADER_FAR_uniforms();

                    static GLuint vbov = 0;
                    static GLuint vbouv = 0;

                    static GLuint billqvbo, billposvbo, billuvvbo = 0;

                    static glm::vec3 last_cam_pos;

                    std::vector<GLfloat> verts;
                    std::vector<GLfloat> uvs;

                    std::vector<GLfloat> billinstances;
                    std::vector<GLfloat> billuvs;

                    float pushup = 0.0f;

                    

                    grid(400, 400, 5, CAMERA_POSITION, [&billinstances, &billuvs, &verts, &uvs, &pushup](float i, float k, float step){




                         verts.insert(verts.end(), {

                            i-step/2.0f, noise_wrap(i-step/2.0f, k-step/2.0f)+pushup ,k-step/2.0f,
                            i+step/2.0f, noise_wrap(i+step/2.0f, k-step/2.0f)+pushup ,k-step/2.0f,
                            i+step/2.0f, noise_wrap(i+step/2.0f, k+step/2.0f)+pushup ,k+step/2.0f,
                            i+step/2.0f, noise_wrap(i+step/2.0f, k+step/2.0f)+pushup ,k+step/2.0f,
                            i-step/2.0f, noise_wrap(i-step/2.0f, k+step/2.0f)+pushup ,k+step/2.0f,
                            i-step/2.0f, noise_wrap(i-step/2.0f, k-step/2.0f)+pushup ,k-step/2.0f,

                                
                                
                                
                                
                                
                                
                            });

                            TextureFace &face = noise_wrap(i,k) > 6 ? BlockTextures[BlockTypes::STONE] : BlockTextures[BlockTypes::GRASS];

                            uvs.insert(uvs.end(), {
                                face.bl.x, face.bl.y,
                                face.tl.x, face.tl.y,
                                face.tr.x, face.tr.y,

                                face.tr.x, face.tr.y,
                                face.br.x, face.br.y,
                                face.bl.x, face.bl.y
                            });

                            float billheight = 2.0f;

                            

                            if(static_cast<float>(rand()) / static_cast<float>(RAND_MAX) < 3.0f)
                            {
                                TextureFace tree(2,0);

                                billinstances.insert(billinstances.end(), {
                                    i, noise_wrap(i, k)+3.0f ,k,
                                });

                                billuvs.insert(billuvs.end(), {
                                    tree.bl.x, tree.bl.y,
                                    tree.tl.x, tree.tl.y,
                                    tree.tr.x, tree.tr.y,
                                    tree.br.x, tree.br.y
                                });
                            }
                    });


                    bool redrawBills = false;

                    GLfloat quadVertices[] = {
                        // Positions    // Corner IDs
                        -3.0f, -3.0f, 0.0f, 0.0f,  // Corner 0
                        3.0f, -3.0f, 0.0f, 1.0f,  // Corner 1
                        3.0f,  3.0f, 0.0f, 2.0f,  // Corner 2
                        -3.0f,  3.0f, 0.0f, 3.0f   // Corner 3
                    };

                    if(vbov == 0 || glm::ivec3(last_cam_pos)/BLOCKCHUNKWIDTH != glm::ivec3(CAMERA_POSITION)/BLOCKCHUNKWIDTH) {
                        redrawBills = true;
                        last_cam_pos = CAMERA_POSITION;
                        glDeleteBuffers(1, &vbov);
                        glDeleteBuffers(1, &vbouv);
                        glGenBuffers(1, &vbov);
                        glGenBuffers(1, &vbouv);

                        bind_geometry(
                        vbov, vbouv, 
                        verts.data(), uvs.data(), 
                        verts.size()*sizeof(GLfloat),
                        uvs.size()*sizeof(GLfloat),
                        SHADER_FAR);
                    } else {
                        bind_geometry_no_upload(vbov, vbouv, SHADER_FAR);
                    }

                    glDrawArrays(GL_TRIANGLES, 0, verts.size());


                    glBindVertexArray(VAO2);

                    glUseProgram(SHADER_BILLBOARD);

                    send_SHADER_BILLBOARD_uniforms();

                    if(billposvbo == 0 || redrawBills) {

                        glDeleteBuffers(1, &billqvbo);
                        glDeleteBuffers(1, &billposvbo);
                        glDeleteBuffers(1, &billuvvbo);
                        glGenBuffers(1, &billqvbo);
                        glGenBuffers(1, &billposvbo);
                        glGenBuffers(1, &billuvvbo);


                        // Quad vertices
                        glBindBuffer(GL_ARRAY_BUFFER, billqvbo);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

                        // Vertex position attribute
                            GLint posAttrib = glGetAttribLocation(SHADER_BILLBOARD, "vertexPosition");
                            glEnableVertexAttribArray(posAttrib);
                            glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);

                            // Corner ID attribute
                            GLint cornerAttrib = glGetAttribLocation(SHADER_BILLBOARD, "cornerID");
                            glEnableVertexAttribArray(cornerAttrib);
                            glVertexAttribPointer(cornerAttrib, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

                        // Instance positions
                        glBindBuffer(GL_ARRAY_BUFFER, billposvbo);
                        glBufferData(GL_ARRAY_BUFFER, billinstances.size() * sizeof(GLfloat), billinstances.data(), GL_STATIC_DRAW);

                        GLint inst_attrib = glGetAttribLocation(SHADER_BILLBOARD, "instancePosition");

                        glEnableVertexAttribArray(inst_attrib);
                        glVertexAttribPointer(inst_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
                        glVertexAttribDivisor(inst_attrib, 1); // Instanced attribute

                        // Instance UVs
                        glBindBuffer(GL_ARRAY_BUFFER, billuvvbo);
                        glBufferData(GL_ARRAY_BUFFER, billuvs.size() * sizeof(GLfloat), billuvs.data(), GL_STATIC_DRAW);


                        GLint uv_attrib_base = glGetAttribLocation(SHADER_BILLBOARD, "instanceUV0");

// First pair of UVs
glEnableVertexAttribArray(uv_attrib_base);
glVertexAttribPointer(uv_attrib_base, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
glVertexAttribDivisor(uv_attrib_base, 1);

// Second pair of UVs
glEnableVertexAttribArray(uv_attrib_base + 1);
glVertexAttribPointer(uv_attrib_base + 1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
glVertexAttribDivisor(uv_attrib_base + 1, 1);

// Third pair of UVs
glEnableVertexAttribArray(uv_attrib_base + 2);
glVertexAttribPointer(uv_attrib_base + 2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));
glVertexAttribDivisor(uv_attrib_base + 2, 1);

// Fourth pair of UVs
glEnableVertexAttribArray(uv_attrib_base + 3);
glVertexAttribPointer(uv_attrib_base + 3, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
glVertexAttribDivisor(uv_attrib_base + 3, 1);


                    } else {



                        // Quad vertices
                        glBindBuffer(GL_ARRAY_BUFFER, billqvbo);

                        // Vertex position attribute
                            GLint posAttrib = glGetAttribLocation(SHADER_BILLBOARD, "vertexPosition");
                            glEnableVertexAttribArray(posAttrib);
                            glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);

                            // Corner ID attribute
                            GLint cornerAttrib = glGetAttribLocation(SHADER_BILLBOARD, "cornerID");
                            glEnableVertexAttribArray(cornerAttrib);
                            glVertexAttribPointer(cornerAttrib, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

                        // Instance positions
                        glBindBuffer(GL_ARRAY_BUFFER, billposvbo);

                        GLint inst_attrib = glGetAttribLocation(SHADER_BILLBOARD, "instancePosition");

                        glEnableVertexAttribArray(inst_attrib);
                        glVertexAttribPointer(inst_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
                        glVertexAttribDivisor(inst_attrib, 1); // Instanced attribute

                        // Instance UVs
                        glBindBuffer(GL_ARRAY_BUFFER, billuvvbo);


                        GLint uv_attrib_base = glGetAttribLocation(SHADER_BILLBOARD, "instanceUV0");

// First pair of UVs
glEnableVertexAttribArray(uv_attrib_base);
glVertexAttribPointer(uv_attrib_base, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
glVertexAttribDivisor(uv_attrib_base, 1);

// Second pair of UVs
glEnableVertexAttribArray(uv_attrib_base + 1);
glVertexAttribPointer(uv_attrib_base + 1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
glVertexAttribDivisor(uv_attrib_base + 1, 1);

// Third pair of UVs
glEnableVertexAttribArray(uv_attrib_base + 2);
glVertexAttribPointer(uv_attrib_base + 2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));
glVertexAttribDivisor(uv_attrib_base + 2, 1);

// Fourth pair of UVs
glEnableVertexAttribArray(uv_attrib_base + 3);
glVertexAttribPointer(uv_attrib_base + 3, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
glVertexAttribDivisor(uv_attrib_base + 3, 1);
                    }

                    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, billinstances.size() / 3);


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
    bool recalc = false;
    if(INPUT_FORWARD) {
        CAMERA_POSITION += CAMERA_DIRECTION * (0.001f + static_cast<float>(DELTA_TIME)) * SPEED_MULTIPLIER;
        recalc = true;
    }
    if(INPUT_LEFT) {
        CAMERA_POSITION -= (-1.0f*CAMERA_RIGHT) * (0.001f + static_cast<float>(DELTA_TIME)) * SPEED_MULTIPLIER;
        recalc = true;
    }
    if(INPUT_RIGHT) {
        CAMERA_POSITION -= CAMERA_RIGHT * (0.001f + static_cast<float>(DELTA_TIME)) * SPEED_MULTIPLIER;
        recalc = true;
    }
    if(INPUT_BACK) {
        CAMERA_POSITION -= CAMERA_DIRECTION * (0.001f + static_cast<float>(DELTA_TIME)) * SPEED_MULTIPLIER;
        recalc = true;
    }
    if(INPUT_JUMP) {
        CAMERA_POSITION += UP * (0.001f + static_cast<float>(DELTA_TIME)) * SPEED_MULTIPLIER;
        recalc = true;
    }
    if(INPUT_SHIFT) {
        CAMERA_POSITION -= UP * (0.001f + static_cast<float>(DELTA_TIME)) * SPEED_MULTIPLIER;
        recalc = true;
    }

    if(recalc) {
        VIEW = glm::lookAt(CAMERA_POSITION, CAMERA_POSITION + CAMERA_DIRECTION, CAMERA_UP);
        MVP = PROJECTION * VIEW * MODEL;
        MODELVIEW = MODEL * VIEW;
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
    ImGui::SliderFloat("Speed", &SPEED_MULTIPLIER, 1.0f, 20.0f);

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

void send_SHADER_FAR_uniforms() {
    GLuint mvp_loc = glGetUniformLocation(SHADER_FAR, "mvp");
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(MVP));

    GLuint cam_pos_loc = glGetUniformLocation(SHADER_FAR, "camPos");
    glUniform3f(cam_pos_loc, CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);

    GLuint brightness_loc = glGetUniformLocation(SHADER_FAR, "brightness");
    glUniform1f(brightness_loc, GLOBAL_BRIGHTNESS);
}

void send_SHADER_STANDARD_uniforms() {
    GLuint mvp_loc = glGetUniformLocation(SHADER_STANDARD, "mvp");
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(MVP));

    GLuint cam_pos_loc = glGetUniformLocation(SHADER_STANDARD, "camPos");
    glUniform3f(cam_pos_loc, CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);

    GLuint brightness_loc = glGetUniformLocation(SHADER_STANDARD, "brightness");
    glUniform1f(brightness_loc, GLOBAL_BRIGHTNESS);
}

void send_SHADER_BILLBOARD_uniforms() {
    GLuint v_loc = glGetUniformLocation(SHADER_BILLBOARD, "v");
    glUniformMatrix4fv(v_loc, 1, GL_FALSE, glm::value_ptr(VIEW));

    GLuint p_loc = glGetUniformLocation(SHADER_BILLBOARD, "p");
    glUniformMatrix4fv(p_loc, 1, GL_FALSE, glm::value_ptr(PROJECTION));

    GLuint m_loc = glGetUniformLocation(SHADER_BILLBOARD, "m");
    glUniformMatrix4fv(m_loc, 1, GL_FALSE, glm::value_ptr(MODEL));

    GLuint cam_pos_loc = glGetUniformLocation(SHADER_BILLBOARD, "camPos");
    glUniform3f(cam_pos_loc, CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);

    GLuint brightness_loc = glGetUniformLocation(SHADER_BILLBOARD, "brightness");
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
        MODELVIEW = MODEL * VIEW;
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


int create_shader_program_with_geometry_shader(GLuint* prog, const char* vfp, const char* ffp, const char* gfilepath) {

std::string vertexText;
    std::string fragText;
    std::string geometryText;
    if(!load_text(gfilepath, geometryText)) {
        std::cerr << "Missing/could not load geometry shade file from path:"
        << gfilepath << std::endl;
        return -1;
    }
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
    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);

    const GLchar *vertexGLChars = vertexText.c_str();
    const GLchar *fragGLChars = fragText.c_str();
    const GLchar* geometryShaderSource = geometryText.c_str();

    glShaderSource(vertexShader, 1, &vertexGLChars, NULL);
    glCompileShader(vertexShader);

    glShaderSource(geometryShader, 1, &geometryShaderSource, nullptr);
    glCompileShader(geometryShader);

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
    glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(geometryShader, 512, nullptr, infoLog);
        std::cerr << "ERROR: Geometry shader compilation failed\n" << infoLog << std::endl;
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
    glAttachShader(*prog, geometryShader);
    glAttachShader(*prog, fragmentShader);
    glLinkProgram(*prog);
    glGetProgramiv(*prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(*prog, 512, NULL, infoLog);
        std::cerr << "Shade prog link err: " << infoLog << std::endl;
        return -1;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);

    return 1;

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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

void bind_geometry(GLuint vbov, GLuint vbouv, const GLfloat *vertices, const GLfloat *uv, size_t vsize, size_t usize, GLuint SHADER)
{
    GLenum error;
    glBindBuffer(GL_ARRAY_BUFFER, vbov);
    glBufferData(GL_ARRAY_BUFFER, vsize, vertices, GL_STATIC_DRAW);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "Bind geom err (vbov): " << error << std::endl;
    }
    GLint pos_attrib = glGetAttribLocation(SHADER, "position");
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbouv);
    glBufferData(GL_ARRAY_BUFFER, usize, uv, GL_STATIC_DRAW);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "Bind geom err (vbouv): " << error << std::endl;
    }
    GLint uv_attrib = glGetAttribLocation(SHADER,"uv");
    glEnableVertexAttribArray(uv_attrib);
    glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void bind_geometry_no_upload(GLuint vbov, GLuint vbouv, GLuint SHADER)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbov);
    GLint pos_attrib = glGetAttribLocation(SHADER, "position");
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbouv);
    GLint uv_attrib = glGetAttribLocation(SHADER, "uv");
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
        5000.0f);
    MVP = PROJECTION * VIEW * MODEL;
    //MODELVIEW = MODEL * VIEW;
    //GLuint mvp_loc = glGetUniformLocation(SHADER_FAR, "mvp");
    //glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(MVP));
}