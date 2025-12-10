#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>
#include <iostream>
#include <set>

#include "utils/sceneparser.h"
#include "camera/camera.h"

class Realtime : public QOpenGLWidget
{
public:
    Realtime(QWidget *parent = nullptr);
    void finish();                                      // Called on program exit
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);

public slots:
    void tick(QTimerEvent* event);                      // Called once per tick of m_timer

protected:
    void initializeGL() override;                       // Called once at the start of the program
    void paintGL() override;                            // Called whenever the OpenGL context changes or by an update() request
    void resizeGL(int width, int height) override;      // Called when window size changes

private:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    // Tick Related Variables
    int m_timer;                                        // Stores timer which attempts to run ~60 times per second
    QElapsedTimer m_elapsedTimer;                       // Stores timer which keeps track of actual time between frames

    // Input Related Variables
    bool m_mouseDown = false;                           // Stores state of left mouse button
    glm::vec2 m_prev_mouse_pos;                         // Stores mouse position
    std::unordered_map<Qt::Key, bool> m_keyMap;         // Stores whether keys are pressed or not

    // Device Correction Variables
    double m_devicePixelRatio;

    // Id stores
    GLuint m_shader, m_shader_bloom, m_shader_blur, m_shader_kuwahara;
    GLuint m_default_fbo, m_fbo, m_rbo, m_fbo_depth;
    GLuint m_color_buffers[2], m_pingpong_fbo[2], m_pingpong_color[2];
    GLuint m_lut_texture;
    GLuint m_kuwahara_fbo, m_kuwahara_tex;

    GLuint m_fullscreen_vbo, m_fullscreen_vao;
    GLuint m_vbo_sphere, m_vbo_cyl, m_vbo_cone, m_vbo_cube, m_vbo_sky;
    GLuint m_vao_sphere, m_vao_cyl, m_vao_cone, m_vao_cube, m_vao_sky;

    GLuint view_ID, proj_ID, model_ID, camera_ID;
    GLuint ambient_k_ID, diffuse_k_ID, specular_k_ID;
    GLuint ambient_ID, diffuse_ID, specular_ID, shininess_ID, light_size_ID;
    GLuint min_fog_ID, max_fog_ID;

    // Vertices vars
    int num_sphere_verts, num_cyl_verts, num_cone_verts, num_cube_verts, num_sky_verts = 0;

    // Random vars
    RenderData m_renderData;
    Camera m_camera;
    int old_param1, old_param2;
    bool initialized = false;
    int m_fbo_width, m_fbo_height, m_screen_width, m_screen_height;
    bool m_parsed = false;
    float m_fog = 0;
    float m_fog_rate = 0.025f;


    // Functions
    void makeFullscreenQuad();
    void makeBloomFBO();
    void loadLUT();
    void setBloom();
    void setKuwahara();
    void createShapes();
    void fillVertices(Shape &shape, GLuint &vbo, GLuint &vao, int &num_verts);
    void phongIllumination(RenderShapeData object);
    void createUniforms();
    glm::mat3 rodrigues(float theta, glm::vec3 axis);

    //Skydome variables and functions
    GLuint m_skyTexture = 0;
    GLuint is_sky_ID;
    void drawSkydome(glm::vec3 camera_pos);
    void initSkydome();

    // Fire variables and functions
    void fireLoop();
    void createCircle(float tessalations, float z);
    void makeCircleSlice(float currentTheta, float nextTheta, float z);
    void makeCircleTile(glm::vec3 bottomRight, glm::vec3 top, glm::vec3 bottomLeft);

    int m_tessalations = 4;
    GLuint m_fire_shader;
    GLuint m_fire_vbo;
    GLuint m_fire_vao;
    std::vector<float> m_vertexData;

    float m_radius = 0.008f;
    struct Particle {
        glm::vec3 position, velocity;
        glm::vec3 color = glm::vec3{0,1,0};
        float life = 1.f;
        float heat = 0.f;
    };
    std::vector<Particle> m_particles;
    GLuint m_pos_vbo;
    std::vector<float> m_pos_data = {};

    GLuint m_color_vbo;
    std::vector<float> m_color_data = {};

    //forces
    float m_gravity = 0.0004f;

    //collisions
    int m_collision_depth = 1;
    float m_bounce_factor = 0.5;

    //particles
    int m_maxParticles = 50000;
    int m_rows = 30;
    int m_cols = 40;
    float m_decay = 0.001;

    //heat
    float m_heat_transfer = 0.5;
    float m_heat_decay = 0.25;

    //bounds
    float m_side_bound = 0.8f;
    float m_ground_bound = 0.0f;
    float m_offset = 0.03;

    /**
     * @brief verifyVAO - prints in the terminal how OpenGL would interpret `triangleData` using the inputted VAO arguments
     * @param triangleData - the vector containing the triangle data
     * @param index - same as glVertexAttribPointer()
     * @param size - same as glVertexAttribPointer()
     * @param stride - same as glVertexAttribPointer()
     * @param offset - same as glVertexAttribPointer()
     */
    void verifyVAO(std::vector<GLfloat> &triangleData, GLuint index, GLsizei size, GLsizei stride, const void* offset) {

        int newStride = int(stride / 4);
        int groupNum = 0;
        int newOffset = static_cast<int>(reinterpret_cast<intptr_t>(offset)) / 4;

        for (int i = newOffset; i < triangleData.size(); i = i + newStride) {
            std::cout << "Group " << groupNum << " of Values for VAO index " << index << std::endl;
            std::cout << "[";
            for (auto j = i; j < i + size; ++j) {
                if (j != i + size - 1) {
                    std::cout << triangleData[j]<< ", ";
                } else {
                    std::cout << triangleData[j]<< "]" << std::endl;
                }
            }
            groupNum = groupNum + 1;
        }
        std::cout << "" << std::endl;
    }
};
