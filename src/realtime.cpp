#include "realtime.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "utils/sceneparser.h"
#include "utils/shaderloader.h"
#include "shape/sphere.h"
#include "shape/cylinder.h"
#include "shape/cone.h"
#include "shape/cube.h"
#include "shape/obj.h"
#include "shape/objloader.h"
#include "camera/camera.h"
#include "lsystem/lsystem.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"

 #include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <fstream>
#include <glm/gtc/matrix_inverse.hpp>

// ================== Rendering the Scene!

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;

    // If you must use this function, do not edit anything above this
}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here
    glDeleteBuffers(1, &m_vbo_sphere);
    glDeleteVertexArrays(1, &m_vao_sphere);

    glDeleteBuffers(1, &m_vbo_cyl);
    glDeleteVertexArrays(1, &m_vao_cyl);

    glDeleteBuffers(1, &m_vbo_cone);
    glDeleteVertexArrays(1, &m_vao_cone);

    glDeleteBuffers(1, &m_vbo_cube);
    glDeleteVertexArrays(1, &m_vao_cube);

    glDeleteBuffers(1, &m_vbo_sky);
    glDeleteVertexArrays(1, &m_vao_sky);

    std::vector<RenderShapeData> &shapes = m_renderData.shapes;
    for (int k = 0; k < shapes.size(); k++) {
        RenderShapeData& object = shapes[k];
        if (object.primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            glDeleteBuffers(1, &object.vbo);
            glDeleteVertexArrays(1, &object.vao);
        }
    }

    glDeleteVertexArrays(1, &m_fullscreen_vao);
    glDeleteBuffers(1, &m_fullscreen_vbo);

    glDeleteProgram(m_shader);
    glDeleteProgram(m_shader_bloom);
    glDeleteProgram(m_shader_blur);
    glDeleteProgram(m_shader_kuwahara);

    glDeleteTextures(2, m_color_buffers);
    glDeleteTextures(2, m_pingpong_color);
    glDeleteRenderbuffers(1, &m_rbo);
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteFramebuffers(1, m_pingpong_fbo);

    this->doneCurrent();
}

void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();
    m_default_fbo = 2;
    m_screen_width = size().width() * m_devicePixelRatio;
    m_screen_height = size().height() * m_devicePixelRatio;
    m_fbo_width = m_screen_width;
    m_fbo_height = m_screen_height;

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    // Initializing GL.
    // GLEW (GL Extension Wrangler) provides access to OpenGL functions.
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    // Allows OpenGL to draw objects appropriately on top of one another
    glEnable(GL_DEPTH_TEST);
    // Tells OpenGL to only draw the front face
    glEnable(GL_CULL_FACE);
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Clear color (may need to change)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Shader setup
    m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/lighting.vert", ":/resources/shaders/lighting.frag");
    m_shader_bloom = ShaderLoader::createShaderProgram(":/resources/shaders/bloom.vert", ":/resources/shaders/bloom.frag");
    m_shader_blur = ShaderLoader::createShaderProgram(":/resources/shaders/blur.vert", ":/resources/shaders/blur.frag");
    createUniforms();
    m_shader_kuwahara = ShaderLoader::createShaderProgram("/Users/christophermok/Desktop/cs1230/projects/proj-flameon/resources/shaders/kuwahara.vert",
                                                          "/Users/christophermok/Desktop/cs1230/projects/proj-flameon/resources/shaders/kuwahara.frag");

    Sphere skySphere;
    // Reasonable tessellation – doesn’t need to match main spheres
    skySphere.updateParams(40, 40);
    fillVertices(skySphere, m_vbo_sky, m_vao_sky, num_sky_verts);

    Obj leaf;
    const std::string path = "src/obj/leaf_1.obj";
    leaf.readOBJ(path);
    fillVertices(leaf, m_vbo_leaf, m_vao_leaf, num_leaf_verts);

    Obj branch;
    const std::string path_1 = "src/obj/branch_new.obj";
    branch.readOBJ(path_1);
    fillVertices(branch, m_vbo_branch, m_vao_branch, num_branch_verts);

    generateLSystem();

    makeFullscreenQuad();
    makeBloomFBO();

    initialized = true;
}

void Realtime::paintGL() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_fbo_width, m_fbo_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shader);

    // Need view, proj, and model in shader for mvp matrix
    glm::mat4 view_mat = m_camera.getViewMatrix();
    glUniformMatrix4fv(view_ID, 1, GL_FALSE, &view_mat[0][0]);

    glm::vec3 camera_pos = glm::vec3(glm::inverse(view_mat)[3]);
    glUniform3f(camera_ID, camera_pos.x, camera_pos.y, camera_pos.z);

    glm::mat4 proj_mat = m_camera.getPerspectiveMatrix();
    glUniformMatrix4fv(proj_ID, 1, GL_FALSE, &proj_mat[0][0]);

    // Phong Id's
    SceneGlobalData global = m_renderData.globalData;
    glUniform1f(ambient_k_ID, global.ka);
    glUniform1f(diffuse_k_ID, global.kd);
    glUniform1f(specular_k_ID, global.ks);

    // Fog uniforms
    glUniform1f(min_fog_ID, settings.fogMin);
    glUniform1f(max_fog_ID, settings.fogMax);

    drawSkydome(camera_pos);

    std::vector<RenderShapeData> &object_list = m_renderData.shapes;
    for (int k = 0; k < object_list.size(); k++) {
        RenderShapeData& object = object_list[k];

        glUniformMatrix4fv(model_ID, 1, GL_FALSE, &object.ctm[0][0]);

        //Don't use instance rendering for other objects
        glUniform1i(useInstancing_ID, 0);

        phongIllumination(object);

        // After all shaders are setup can actually draw the objects
        if (object.primitive.type == PrimitiveType::PRIMITIVE_SPHERE) {
            glBindVertexArray(m_vao_sphere);
            glDrawArrays(GL_TRIANGLES, 0, num_sphere_verts);
        }
        else if (object.primitive.type == PrimitiveType::PRIMITIVE_CYLINDER) {
            glBindVertexArray(m_vao_cyl);
            glDrawArrays(GL_TRIANGLES, 0, num_cyl_verts);
        }
        else if (object.primitive.type == PrimitiveType::PRIMITIVE_CONE) {
            glBindVertexArray(m_vao_cone);
            glDrawArrays(GL_TRIANGLES, 0, num_cone_verts);
        }
        else if (object.primitive.type == PrimitiveType::PRIMITIVE_CUBE) {
            glBindVertexArray(m_vao_cube);
            glDrawArrays(GL_TRIANGLES, 0, num_cube_verts);
        }
        else if (object.primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            glBindVertexArray(object.vao);
            glDrawArrays(GL_TRIANGLES, 0, object.num_verts);
        }
        else if (object.primitive.type == PrimitiveType::BRANCH){
            glBindVertexArray(m_vao_branch);
            glDrawArrays(GL_TRIANGLES, 0, num_branch_verts);
        }
        else if (object.primitive.type == PrimitiveType::LEAF){
            glBindVertexArray(m_vao_leaf);
            glDrawArrays(GL_TRIANGLES, 0, num_leaf_verts);
        }
    }

    glUniform1i(useInstancing_ID, 1);
    drawLSystem();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_default_fbo);
    glViewport(0, 0, m_screen_width, m_screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //setBloom();
    setKuwahara();
}

void Realtime::createShapes() {
    std::set<int> shape_exists;
    std::vector<RenderShapeData> &shapes = m_renderData.shapes;

    for (int k = 0; k < shapes.size(); k++) {
        RenderShapeData& object = shapes[k];
        PrimitiveType type = object.primitive.type;

        // Each shape type gets one vbo/vao, NOT multiple per shape
        if (shape_exists.find(int(type)) == shape_exists.end()) {
            if (type == PrimitiveType::PRIMITIVE_SPHERE) {
                Sphere single_sphere;
                single_sphere.updateParams(settings.shapeParameter1, settings.shapeParameter2);
                fillVertices(single_sphere, m_vbo_sphere, m_vao_sphere, num_sphere_verts);
            }
            else if (type == PrimitiveType::PRIMITIVE_CYLINDER) {
                Cylinder single_cyl;
                single_cyl.updateParams(settings.shapeParameter1, settings.shapeParameter2);
                fillVertices(single_cyl, m_vbo_cyl, m_vao_cyl, num_cyl_verts);
            }
            else if (type == PrimitiveType::PRIMITIVE_CONE) {
                Cone single_cone;
                single_cone.updateParams(settings.shapeParameter1, settings.shapeParameter2);
                fillVertices(single_cone, m_vbo_cone, m_vao_cone, num_cone_verts);
            }
            else if (type == PrimitiveType::PRIMITIVE_CUBE) {
                Cube single_cube;
                single_cube.updateParams(settings.shapeParameter1, settings.shapeParameter2);
                fillVertices(single_cube, m_vbo_cube, m_vao_cube, num_cube_verts);
            }
            else if (type == PrimitiveType::LEAF){
                Obj leaf;
                const std::string path = "src/obj/leaf_1.obj";
                leaf.readOBJ(path);
                fillVertices(leaf, m_vbo_leaf, m_vao_leaf, num_leaf_verts);
            }
            else if(type == PrimitiveType::BRANCH){
                Obj branch;
                const std::string path = "src/obj/branch_2.obj";
                branch.readOBJ(path);
                fillVertices(branch, m_vbo_branch, m_vao_branch, num_branch_verts);
            }
            shape_exists.insert(int(type));
        }

        // Meshes can't share one vbo/vao like the other shapes because they are each different
        if (type == PrimitiveType::PRIMITIVE_MESH) {
            object.shape->updateParams(settings.shapeParameter1, settings.shapeParameter2);
            fillVertices(*object.shape, object.vbo, object.vao, object.num_verts);
        }
    }

    old_param1 = settings.shapeParameter1;
    old_param2 = settings.shapeParameter2;

    //Tree instance rendering
    // if (m_branchInstanceVBO != 0 || m_leafInstanceVBO != 0) {
    //     setupTreeInstances();
    // }
}

void Realtime::generateLSystem(){
    m_treeData.clear();
    std::cout<<"Generating tree"<<std::endl;
    Lsystem tree = Lsystem();
    //int iterations = settings.shapeParameter1;
    int iterations = 4;

    std::cout<<"Iterations: "<<iterations<<std::endl;

    float step = 0.8f;
    float angleStep = glm::radians(25.f);

    // //Tree 1
    // std::string axiom = "F";
    // tree.insertRule('F', "F[+F]F[-F]F");

    //Tree 2
    std::string axiom = "X";
    tree.insertRule('X', "F+[[X]-X]-F[-FX]+X");
    tree.insertRule('F', "FF");

    // //Tree 3
    // std::string axiom = "F";
    // tree.insertRule('F', "F[+FF][-FF]F[-F][+F]F");

    std::string word = tree.generate(axiom);

    for(int i = 0; i < iterations; i++){
        word = tree.generate(word);
    }


    Turtle head;
    head.pos = glm::vec4(0,0,0,1);
    head.angle = glm::radians(90.f);
    head.ctm = glm::mat4(1);

    glm::mat4 ctm(1);
    glm::vec3 rotationAxis(1,0,0);
    glm::vec3 scaleFac(0.9,0.9,0.9);

    std::stack<Turtle> headStack;

    float rand_yaw;
    float randFac = settings.shapeParameter2 / 1.f;

    // prevTreeLength = settings.shapeParameter1;
    // prevTreeRand = settings.shapeParameter2;

    for (char c : word) {
        rand_yaw = 15 * (random()%360 + 1);
        switch (c) {
        case 'F':{

            head.ctm = head.ctm * glm::rotate(glm::radians(rand_yaw), glm::vec3(0,1,0));

            glm::mat4 segCTM = head.ctm * glm::translate(glm::vec3(0.f, step * 0.5f, 0.f));

            SceneMaterial material;
            material.cDiffuse = glm::vec4(0.5f, 0.0f, 0.7f, 1.0f);
            material.cSpecular = glm::vec4(0.5, 0.5, 0.5, 1.0);
            material.cAmbient = glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
            m_branchMaterial = material;
            ScenePrimitive prim = {PrimitiveType::BRANCH, material};
            RenderShapeData new_branch = {prim, head.ctm};
            new_branch.shape = new Obj();
            m_treeData.push_back(new_branch);
            head.ctm = head.ctm * glm::translate(glm::vec3(0.f, step, 0.f));
            break;
        }
        case '+':
            head.ctm = head.ctm * glm::rotate(angleStep, rotationAxis);
            break;

        case '-':
            head.ctm = head.ctm * glm::rotate(-angleStep, rotationAxis);
            break;

        case '[':
            headStack.push(head);
            break;

        case ']':{
            head.ctm = head.ctm * glm::translate(glm::vec3(0.f, step*1.5, 0.f));
            head.ctm = head.ctm * glm::rotate(glm::radians(90.f), glm::vec3(0,1,0)); //fix leaf orientation

            SceneMaterial material;
            material.cDiffuse = glm::vec4(0.0f, 0.9f, 0.1f, 1.0f);
            material.cSpecular = glm::vec4(0.5, 0.5, 0.5, 1.0);
            material.cAmbient = glm::vec4(0.0f, 0.5f, 0.2f, 1.0f);
            //
            m_leafMaterial = material;
            ScenePrimitive prim = {PrimitiveType::LEAF, material};
            RenderShapeData new_leaf = {prim, head.ctm};
            new_leaf.shape = new Obj();
            m_treeData.push_back(new_leaf);

            if (!headStack.empty()) {
                head = headStack.top();
                headStack.pop();
            }
            break;
        }
        default:
            break;
        }
    }

}

void Realtime::drawLSystem(){
    for(int i = 0; i < m_treeData.size(); i++){
        phongIllumination(m_treeData[i]);
        switch (m_treeData[i].primitive.type) {
        case PrimitiveType::BRANCH:
            glUniformMatrix4fv(model_ID, 1, GL_FALSE, &m_treeData[i].ctm[0][0]);

            glBindVertexArray(m_vao_branch);
            glDrawArrays(GL_TRIANGLES, 0, num_branch_verts);

            glBindVertexArray(0);
            break;
        case PrimitiveType::LEAF:
            //drawLeaf(m_treeData[i].second);
            glUniformMatrix4fv(model_ID, 1, GL_FALSE, &m_treeData[i].ctm[0][0]);
            //std::cout<<"Leaf drawn"<<std::endl;
            glBindVertexArray(m_vao_leaf);
            glDrawArrays(GL_TRIANGLES, 0, num_leaf_verts);

            glBindVertexArray(0);

            break;
        default:
            break;
        }
    }
}

void Realtime::drawSkydome(glm::vec3 camera_pos){
    glDepthMask(GL_FALSE);
    // See inside the sphere
    glDisable(GL_CULL_FACE);

    // Tell shader this is sky
    glUniform1i(is_sky_ID, 1);

    // Center dome on camera in world space
    float radius = 199.f; // large enough vs. your scene scale
    glm::mat4 model_sky =
        glm::translate(camera_pos) *
        glm::scale(glm::vec3(radius));

    glUniformMatrix4fv(model_ID, 1, GL_FALSE, &model_sky[0][0]);

    // Optional: set sky colors
    GLint skyTop_ID    = glGetUniformLocation(m_shader, "u_skyTopColor");
    GLint skyBottom_ID = glGetUniformLocation(m_shader, "u_skyBottomColor");
    //glUniform3f(skyTop_ID,    0.05f, 0.1f, 0.4f); // dark-ish blue
    glUniform3f(skyTop_ID,    1.0f, 1.0f, 1.0f); // dark-ish blue
    glUniform3f(skyBottom_ID, 0.6f,  0.8f, 1.0f); // horizon

    glBindVertexArray(m_vao_sky);
    glDrawArrays(GL_TRIANGLES, 0, num_sky_verts);
    glBindVertexArray(0);

    // Reset state
    glUniform1i(is_sky_ID, 0);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void Realtime::fillVertices(Shape &shape, GLuint &vbo, GLuint &vao, int &num_verts) {
    std::vector<GLfloat> verts = shape.generateShape();
    // Position + Normal = One vert
    num_verts = verts.size() / 6;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, reinterpret_cast<void*>(3 * sizeof(GLfloat)));
}

void Realtime::phongIllumination(RenderShapeData object) {
    SceneMaterial material = object.primitive.material;

    // Phong Id's
    glUniform4f(ambient_ID, material.cAmbient.x, material.cAmbient.y, material.cAmbient.z, material.cAmbient.w);
    glUniform4f(diffuse_ID, material.cDiffuse.x, material.cDiffuse.y, material.cDiffuse.z, material.cDiffuse.w);
    glUniform4f(specular_ID, material.cSpecular.x, material.cSpecular.y, material.cSpecular.z, material.cSpecular.w);
    glUniform1f(shininess_ID, material.shininess);

    std::vector<SceneLightData> lights = m_renderData.lights;
    glUniform1i(light_size_ID, lights.size());
    for (int i = 0; i < lights.size(); i++) {
        SceneLightData light = lights[i];

        GLuint light_type_ID = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].type").c_str());
        glUniform1i(light_type_ID, (int)light.type);

        GLuint light_pos_ID = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].pos").c_str());
        glUniform4f(light_pos_ID, light.pos.x, light.pos.y, light.pos.z, light.pos.w);

        GLuint light_dir_ID = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].dir").c_str());
        glUniform4f(light_dir_ID, light.dir.x, light.dir.y, light.dir.z, light.dir.w);

        GLuint light_color_ID = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].color").c_str());
        glUniform4f(light_color_ID, light.color.x, light.color.y, light.color.z, light.color.w);

        GLuint light_func_ID = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].function").c_str());
        glUniform3f(light_func_ID, light.function.x, light.function.y, light.function.z);

        GLuint light_angle_ID = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].angle").c_str());
        glUniform1f(light_angle_ID, light.angle);

        GLuint light_penumbra_ID = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].penumbra").c_str());
        glUniform1f(light_penumbra_ID, light.penumbra);
    }
}


void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    m_screen_width = size().width() * m_devicePixelRatio;
    m_screen_height = size().height() * m_devicePixelRatio;
    m_fbo_width = m_screen_width;
    m_fbo_height = m_screen_height;
    // Students: anything requiring OpenGL calls when the program starts should be done here
}

void Realtime::sceneChanged() {
    // Clear render data before parsing again to avoid accumulation
    m_renderData = RenderData{};
    SceneParser parser;
    parser.parse(settings.sceneFilePath, m_renderData);

    // Create new vbo/vaos and then default them to 0
    createShapes();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_camera.camera = m_renderData.cameraData;
    m_camera.width = size().width();
    m_camera.height = size().height();

    update(); // asks for a PaintGL() call to occur
}

void Realtime::settingsChanged() {
    // Function gets called when first running program - don't want to adjust settings yet then
    if (!initialized) { return; }

    if (old_param1 != settings.shapeParameter1 || old_param2 != settings.shapeParameter2) {
        createShapes();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    update(); // asks for a PaintGL() call to occur
}

void Realtime::createUniforms() {
    view_ID = glGetUniformLocation(m_shader, "view_mat");
    proj_ID = glGetUniformLocation(m_shader, "proj_mat");
    model_ID = glGetUniformLocation(m_shader, "model_mat");
    camera_ID = glGetUniformLocation(m_shader, "camera_pos");

    ambient_k_ID = glGetUniformLocation(m_shader, "ka");
    diffuse_k_ID = glGetUniformLocation(m_shader, "kd");
    specular_k_ID = glGetUniformLocation(m_shader, "ks");

    ambient_ID = glGetUniformLocation(m_shader, "ambient");
    diffuse_ID = glGetUniformLocation(m_shader, "diffuse");
    specular_ID = glGetUniformLocation(m_shader, "specular");
    shininess_ID = glGetUniformLocation(m_shader, "shininess");
    light_size_ID = glGetUniformLocation(m_shader, "light_size");

    min_fog_ID = glGetUniformLocation(m_shader, "min_dist");
    max_fog_ID = glGetUniformLocation(m_shader, "max_dist");

    useInstancing_ID = glGetUniformLocation(m_shader, "useInstancing");

    is_sky_ID = glGetUniformLocation(m_shader, "u_isSky");
}

void Realtime::makeFullscreenQuad() {
    // Draw quad onto screen for fbos to work
    std::vector<GLfloat> fullscreen_quad_data =
        { //     POSITIONS + UV    //
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f
        };
    glGenBuffers(1, &m_fullscreen_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreen_vbo);
    glBufferData(GL_ARRAY_BUFFER, fullscreen_quad_data.size()*sizeof(GLfloat), fullscreen_quad_data.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &m_fullscreen_vao);
    glBindVertexArray(m_fullscreen_vao);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Realtime::makeBloomFBO() {
    // Color buffers
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    // Need color buffer for bright colors and normal colors
    glGenTextures(2, m_color_buffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, m_color_buffers[i]);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_fbo_width, m_fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_fbo_width, m_fbo_height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Clamp to avoid blur filtering using repeated textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Need to use two different color attachments
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_color_buffers[i], 0);
    }

    // Renderbuffer
    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_fbo_width, m_fbo_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
    // Tells opengl there are multiple color buffers
    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, m_default_fbo);

    // Ping-pong buffers to blur the color buffers together
    glGenFramebuffers(2, m_pingpong_fbo);
    glGenTextures(2, m_pingpong_color);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingpong_fbo[i]);
        glBindTexture(GL_TEXTURE_2D, m_pingpong_color[i]);

        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_fbo_width, m_fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_fbo_width, m_fbo_height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpong_color[i], 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }
    // With two more fbo's, default increase from 2 -> 4
    m_default_fbo = 4;
    glBindFramebuffer(GL_FRAMEBUFFER, m_default_fbo);
}

void Realtime::setKuwahara() {
    glUseProgram(m_shader_kuwahara);

    // Bind default framebuffer (screen)
    glBindFramebuffer(GL_FRAMEBUFFER, m_default_fbo);
    glViewport(0, 0, m_screen_width, m_screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint bloom_ID = glGetUniformLocation(m_shader_kuwahara, "bloom");
    glUniform1i(bloom_ID, settings.bloom);

    // Bind the scene color buffer as input
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_color_buffers[0]);

    GLint texLoc = glGetUniformLocation(m_shader_kuwahara, "u_tex");
    glUniform1i(texLoc, 0);

    GLint texelLoc = glGetUniformLocation(m_shader_kuwahara, "u_texelSize");
    glUniform2f(texelLoc,
                1.0f / float(m_fbo_width),
                1.0f / float(m_fbo_height));

    glBindVertexArray(m_fullscreen_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
}


void Realtime::setBloom() {
    glUseProgram(m_shader_blur);
    GLuint texture_ID = glGetUniformLocation(m_shader_blur, "tex");
    glUniform1i(texture_ID, 0);

    bool horizontal = true;
    bool first_iteration = true;
    unsigned int amount = 10;
    for (unsigned int i = 0; i < amount; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingpong_fbo[horizontal]);
        glViewport(0, 0, m_fbo_width, m_fbo_height);
        GLuint horizontal_ID = glGetUniformLocation(m_shader_blur, "horizontal");
        glUniform1i(horizontal_ID, horizontal);

        // Alternate the blurring on pingpong buffers but first blurring is on normal scene
        glActiveTexture(GL_TEXTURE0);
        if (first_iteration) {
            glBindTexture(GL_TEXTURE_2D, m_color_buffers[1]);
            first_iteration = false;
        } else {
            glBindTexture(GL_TEXTURE_2D, m_pingpong_color[!horizontal]);
        }
        glBindVertexArray(m_fullscreen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        horizontal = !horizontal;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_default_fbo);
    glViewport(0, 0, m_screen_width, m_screen_height);

    glUseProgram(m_shader_bloom);

    GLuint bloom_ID = glGetUniformLocation(m_shader_bloom, "bloom");
    glUniform1i(bloom_ID, settings.bloom);
    GLuint scene_ID = glGetUniformLocation(m_shader_bloom, "scene");
    glUniform1i(scene_ID, 0);
    GLuint blur_ID = glGetUniformLocation(m_shader_bloom, "blur");
    glUniform1i(blur_ID, 1);
    GLuint exposure_ID = glGetUniformLocation(m_shader_bloom, "exposure");
    glUniform1f(exposure_ID, settings.exposure);

    glBindVertexArray(m_fullscreen_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_color_buffers[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_pingpong_color[!horizontal]);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
}

// ================== Camera Movement!

void Realtime::keyPressEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = true;
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = true;
        m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = false;
    }
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
    if (m_mouseDown) {
        int posX = event->position().x();
        int posY = event->position().y();
        int deltaX = posX - m_prev_mouse_pos.x;
        int deltaY = posY - m_prev_mouse_pos.y;
        m_prev_mouse_pos = glm::vec2(posX, posY);

        float amount = 0.01f;
        SceneCameraData& cam = m_camera.camera;
        glm::vec3 final_pos = cam.pos;

        glm::vec3 look = glm::normalize(cam.look);
        glm::vec3 up = glm::normalize(cam.up);

        // Use deltaX and deltaY here to rotate
        glm::mat3 rod_x = rodrigues(deltaX * amount, up);
        look = glm::normalize(rod_x * look);

        glm::vec3 right = glm::normalize(glm::cross(look, up));
        glm::mat3 rod_y = rodrigues(deltaY * amount, right);
        look = glm::normalize(rod_y * look);

        cam.look = glm::vec4(look, 0.f);

        update(); // asks for a PaintGL() call to occur
    }
}

glm::mat3 Realtime::rodrigues(float theta, glm::vec3 axis) {
    glm::vec3 u = glm::normalize(axis);
    float cos = std::cos(theta);
    float sin = std::sin(theta);

    glm::mat3 result(
        cos + u.x*u.x*(1.0f - cos), u.x*u.y*(1.0f - cos) - u.z*sin, u.x*u.z*(1.0f - cos) + u.y*sin,
        u.x*u.y*(1.0f - cos) + u.z*sin, cos + u.y*u.y*(1.0f - cos), u.y*u.z*(1.0f - cos) - u.x*sin,
        u.x*u.z*(1.0f - cos) - u.y*sin, u.y*u.z*(1.0f - cos) + u.x*sin, cos + u.z*u.z*(1.0f - cos)
    );
    return result;
}

void Realtime::timerEvent(QTimerEvent *event) {
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    float amount = 5.0f * deltaTime;
    SceneCameraData& cam = m_camera.camera;
    glm::vec3 final_pos = cam.pos;

    glm::vec3 look = glm::normalize(cam.look);
    glm::vec3 up = glm::normalize(cam.up);
    glm::vec3 right = glm::normalize(glm::cross(look, up));

    // Use deltaTime and m_keyMap here to move around
    if (m_keyMap[Qt::Key_W]) {
        final_pos += look * amount;
    }
    if (m_keyMap[Qt::Key_S]) {
        final_pos -= look * amount;
    }
    if (m_keyMap[Qt::Key_D]) {
        final_pos += right * amount;
    }
    if (m_keyMap[Qt::Key_A]) {
        final_pos -= right * amount;
    }
    if (m_keyMap[Qt::Key_Space]) {
        final_pos += up * amount;
    }
    if (m_keyMap[Qt::Key_Control]) {
        final_pos -= up * amount;
    }

    if (m_keyMap[Qt::Key_O]) { // or whatever
        exportTreeToOBJ("tree_export.obj");
    }
    cam.pos = glm::vec4(final_pos, 1.0f);

    update(); // asks for a PaintGL() call to occur
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = 1024;
    int fixedHeight = 768;

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}

void Realtime::exportTreeToOBJ(const std::string &filepath) {
    // Make sure we actually HAVE a tree
    if (m_treeData.empty()) {
        std::cerr << "exportTreeToOBJ: m_treeData is empty, call generateLSystem() first.\n";
        return;
    }

    std::ofstream out(filepath);
    if (!out) {
        std::cerr << "Failed to open " << filepath << " for writing\n";
        return;
    }

    // 1) Load base branch + leaf geometry (local space)
    Obj branchObj;
    Obj leafObj;

    // Use the same files you use for rendering
    const std::string branchPath = "src/obj/branch_new.obj";
    const std::string leafPath   = "src/obj/leaf_2.obj";

    branchObj.readOBJ(branchPath);
    leafObj.readOBJ(leafPath);

    std::vector<GLfloat> branchVerts = branchObj.generateShape(); // [px,py,pz, nx,ny,nz, ...]
    std::vector<GLfloat> leafVerts   = leafObj.generateShape();

    auto splitVerts = [](const std::vector<GLfloat> &verts,
                         std::vector<glm::vec3> &positions,
                         std::vector<glm::vec3> &normals) {
        positions.clear();
        normals.clear();
        positions.reserve(verts.size() / 6);
        normals.reserve(verts.size() / 6);

        for (size_t i = 0; i < verts.size(); i += 6) {
            positions.emplace_back(verts[i + 0], verts[i + 1], verts[i + 2]);
            normals.emplace_back  (verts[i + 3], verts[i + 4], verts[i + 5]);
        }
    };

    std::vector<glm::vec3> branchPos, branchNorm;
    std::vector<glm::vec3> leafPos, leafNorm;
    splitVerts(branchVerts, branchPos, branchNorm);
    splitVerts(leafVerts,   leafPos,   leafNorm);

    // 2) Walk over all L-system nodes and bake them out
    int globalVertexOffset = 1; // OBJ is 1-based

    auto writeInstanceMesh = [&](const RenderShapeData &node,
                                 const std::vector<glm::vec3> &basePos,
                                 const std::vector<glm::vec3> &baseNorm,
                                 int &globalOffset)
    {
        glm::mat4 M = node.ctm;
        glm::mat3 N = glm::transpose(glm::inverse(glm::mat3(M)));

        int localVertCount = static_cast<int>(basePos.size());

        // Optionally write a group/object name so it’s easier to inspect later
        // out << "g ";
        // if (node.primitive.type == PrimitiveType::BRANCH) out << "branch\n";
        // else out << "leaf\n";

        // 2.1) vertices + normals
        for (int i = 0; i < localVertCount; ++i) {
            glm::vec3 p = basePos[i];
            glm::vec3 n = baseNorm[i];

            glm::vec4 wp4 = M * glm::vec4(p, 1.0f);
            glm::vec3 wp  = glm::vec3(wp4) / wp4.w;

            glm::vec3 wn  = glm::normalize(N * n);

            out << "v "  << wp.x << " " << wp.y << " " << wp.z << "\n";
            out << "vn " << wn.x << " " << wn.y << " " << wn.z << "\n";
        }

        // 2.2) faces: we assume 3 verts per triangle in order
        for (int i = 0; i < localVertCount; i += 3) {
            int i0 = globalOffset + i;
            int i1 = globalOffset + i + 1;
            int i2 = globalOffset + i + 2;

            // f v//vn v//vn v//vn  (same index used for v and vn)
            out << "f "
                << i0 << "//" << i0 << " "
                << i1 << "//" << i1 << " "
                << i2 << "//" << i2 << "\n";
        }

        globalOffset += localVertCount;
    };

    for (const RenderShapeData &node : m_treeData) {
        if (node.primitive.type == PrimitiveType::BRANCH) {
            if (!branchPos.empty()) {
                writeInstanceMesh(node, branchPos, branchNorm, globalVertexOffset);
            }
        } else if (node.primitive.type == PrimitiveType::LEAF) {
            if (!leafPos.empty()) {
                writeInstanceMesh(node, leafPos, leafNorm, globalVertexOffset);
            }
        }
    }

    out.close();
    std::cout << "Exported tree to OBJ: " << filepath << std::endl;
}
