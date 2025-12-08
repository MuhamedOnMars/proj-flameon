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
#include <stack>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

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
    void exportTreeToOBJ(const std::string &filepath);

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
    GLuint m_default_fbo, m_fbo, m_rbo;
    GLuint m_color_buffers[2], m_pingpong_fbo[2], m_pingpong_color[2];

    GLuint m_fullscreen_vbo, m_fullscreen_vao;
    GLuint m_vbo_sphere, m_vbo_cyl, m_vbo_cone, m_vbo_cube, m_vbo_leaf, m_vbo_branch, m_vbo_sky = 0;
    GLuint m_vao_sphere, m_vao_cyl, m_vao_cone, m_vao_cube, m_vao_leaf, m_vao_branch, m_vao_sky = 0;

    GLuint view_ID, proj_ID, model_ID, camera_ID;
    GLuint ambient_k_ID, diffuse_k_ID, specular_k_ID;
    GLuint ambient_ID, diffuse_ID, specular_ID, shininess_ID, light_size_ID;
    GLuint min_fog_ID, max_fog_ID;

    GLuint m_skyTexture = 0;

    //Instance rendering
    GLuint m_branchInstanceVBO = 0;
    GLuint m_leafInstanceVBO   = 0;
    int m_numBranchInstances = 0;
    int m_numLeafInstances   = 0;
    GLuint useInstancing_ID;
    SceneMaterial m_branchMaterial;
    SceneMaterial m_leafMaterial;

    // Vertices vars
    int num_sphere_verts, num_cyl_verts, num_cone_verts, num_cube_verts, num_leaf_verts, num_branch_verts, num_sky_verts;
    std::vector<float> m_leafData;
    std::vector<float> m_branchData;

    // Random vars
    RenderData m_renderData;
    Camera m_camera;
    int old_param1, old_param2;
    bool initialized = false;
    int m_fbo_width, m_fbo_height, m_screen_width, m_screen_height;
    std::vector<RenderShapeData> m_treeData; //For L-Systems
    GLuint is_sky_ID;


    // Functions
    void makeFullscreenQuad();
    void makeBloomFBO();
    void setBloom();
    void setKuwahara();
    void createShapes();
    void fillVertices(Shape &shape, GLuint &vbo, GLuint &vao, int &num_verts);
    void phongIllumination(RenderShapeData object);
    void createUniforms();
    glm::mat3 rodrigues(float theta, glm::vec3 axis);
    //L-System Functions
    void generateLSystem();
    void drawLSystem();
    void setupTreeInstances();
    void initBranchGeometryAndInstances();
    void initLeafGeometryAndInstances();
    void drawSkydome(glm::vec3 camera_pos);


    //OBJ export
    // std::vector<GLfloat> m_branchVerts;
    // std::vector<GLfloat> m_leafVerts;

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
