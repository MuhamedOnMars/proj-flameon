#include "realtime.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "utils/sceneparser.h"
#include "utils/shaderloader.h"
#include "shape/sphere.h"
#include "shape/cylinder.h"
#include "shape/cone.h"
#include "shape/cube.h"
#include "shape/objloader.h"
#include "camera/camera.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"

 #include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/transform.hpp>

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

    std::vector<RenderShapeData> &shapes = m_renderData.shapes;
    for (int k = 0; k < shapes.size(); k++) {
        RenderShapeData& object = shapes[k];
        if (object.primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            glDeleteBuffers(1, &object.vbo);
            glDeleteVertexArrays(1, &object.vao);
        }
    }

    if (m_skyTexture) {
        glDeleteTextures(1, &m_skyTexture);
    }

    glDeleteVertexArrays(1, &m_fullscreen_vao);
    glDeleteBuffers(1, &m_fullscreen_vbo);

    glDeleteProgram(m_shader);
    glDeleteProgram(m_shader_bloom);
    glDeleteProgram(m_shader_blur);
    glDeleteProgram(m_fire_shader);
    glDeleteProgram(m_shader_kuwahara);

    glDeleteTextures(2, m_color_buffers);
    glDeleteTextures(2, m_pingpong_color);
    glDeleteRenderbuffers(1, &m_rbo);
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteFramebuffers(1, m_pingpong_fbo);

    this->doneCurrent();
}


void insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

void Realtime::makeCircleTile(glm::vec3 bottomRight, glm::vec3 top, glm::vec3 bottomLeft) {
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, top);
    insertVec3(m_vertexData, bottomRight);
}

void Realtime::makeCircleSlice(float currentTheta, float nextTheta, float z) {
    if (m_radius <= 0.f) return; // nothing to draw

    glm::vec3 center     = {0.f, 0.f, z};
    glm::vec3 edge1      = {m_radius * cos(currentTheta), m_radius * sin(currentTheta), z};
    glm::vec3 edge2      = {m_radius * cos(nextTheta),    m_radius * sin(nextTheta),    z};

    // One triangle: center, edge1, edge2
    makeCircleTile(edge1, center, edge2);
}


void Realtime::createCircle(float tessalations, float z) {
    float step_size = (2*M_PI)/tessalations;
    for (int i = 0; i<tessalations; ++i) {
        float currentTheta = i*step_size;
        float nextTheta = (i+1)*step_size;
        makeCircleSlice(currentTheta, nextTheta, z);
    }
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
    m_fire_shader = ShaderLoader::createShaderProgram(":/resources/shaders/fire.vert", ":/resources/shaders/fire.frag");
    m_shader_kuwahara = ShaderLoader::createShaderProgram(":/resources/shaders/kuwahara.vert", ":/resources/shaders/kuwahara.frag");

    createUniforms();

    //Skydome
    Sphere skySphere;
    // Reasonable tessellation – doesn’t need to match main spheres
    skySphere.updateParams(40, 40);
    fillVertices(skySphere, m_vbo_sky, m_vao_sky, num_sky_verts);
    initSkydome();

    //fire
    createCircle(m_tessalations, -0.f);
    //instance particles
    for(int i = -m_rows; i<m_rows;++i) {
        for(int j = -m_cols; j<m_cols;++j) {
            float z = ((rand() % 1000) / 1000.f - 0.5f) * 0.15f;  // ±0.075 depth, chat
            Particle p{glm::vec3{i*(m_offset), j*(m_offset)+2.f,z}};
            p.heat = 0.4f + 0.3f * ((rand()%1000)/1000.f); //random heat on spawn, chat
            m_particles.push_back(p);
        }
    }

    //initial positions/offsets + colors
    for(int i = 0; i<m_particles.size();++i) {
        m_pos_data.push_back(m_particles[i].position.x);
        m_pos_data.push_back(m_particles[i].position.y);
        m_pos_data.push_back(m_particles[i].position.z);

        m_color_data.push_back(m_particles[i].color.x);
        m_color_data.push_back(m_particles[i].color.y);
        m_color_data.push_back(m_particles[i].color.z);
    }

    //positions/offsets
    glGenBuffers(GLuint(1.f), &m_pos_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo);
    glBufferData(GL_ARRAY_BUFFER, 50000*3*sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_pos_data.size()*sizeof(GLfloat), m_pos_data.data());

    //colors
    glGenBuffers(GLuint(1.f), &m_color_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_color_vbo);
    glBufferData(GL_ARRAY_BUFFER, 50000*3*sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_color_data.size()*sizeof(GLfloat), m_color_data.data());

    //generate vao
    glGenBuffers(GLuint(1.f), &m_fire_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_fire_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_vertexData.size(), m_vertexData.data(), GL_STATIC_DRAW);
    //fire vao
    glGenVertexArrays(GLuint(1.f), &m_fire_vao);
    glBindVertexArray(m_fire_vao);
    //fire vao attributes
    glEnableVertexAttribArray(0); //position
    glEnableVertexAttribArray(1); //offsets
    glEnableVertexAttribArray(2); //color

    glBindBuffer(GL_ARRAY_BUFFER, m_fire_vbo);
    glVertexAttribPointer(0, 3.f, GL_FLOAT, GL_FALSE,3*sizeof(GLfloat),reinterpret_cast<void*>(0)); //position
    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo);
    glVertexAttribPointer(1, 3.f, GL_FLOAT, GL_FALSE,0,reinterpret_cast<void*>(0)); //offsets
    glBindBuffer(GL_ARRAY_BUFFER, m_color_vbo);
    glVertexAttribPointer(2, 3.f, GL_FLOAT, GL_FALSE,0,reinterpret_cast<void*>(0)); //colors

    //unbind fire vao
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    makeFullscreenQuad();
    makeBloomFBO();
    loadLUT();

    initialized = true;
}

bool checkOverlap(glm::vec2 c1, glm::vec2 c2, float r1, float r2) {
    return fabs((c1.x-c2.x)*(c1.x-c2.x) + (c1.y-c2.y)*(c1.y-c2.y)) <= (r1+r2)*(r1+r2);
}

void Realtime::fireLoop() {
    //movement + gravity
    for(int a = 0; a<m_particles.size(); ++a) {
        float wiggle = 0.0005f;
        m_particles[a].velocity.x += wiggle * (rand()%2000/1000.f - 1.f); //jitter

        int index1 = 3*a;
        //gravity
        m_particles[a].velocity.y -= m_gravity;

        //x-axis move
        m_pos_data[index1 + 0] += m_particles[a].velocity.x;

        //ground check
        if(m_pos_data[index1 + 1] + m_particles[a].velocity.y < -m_ground_bound + m_radius) {
            m_pos_data[index1 +1] = -m_ground_bound +m_radius;
            m_particles[a].velocity.y *= -m_bounce_factor;
            m_particles[a].velocity.x *= 0.95f;
        }
        else {
            m_pos_data[index1 + 1] += m_particles[a].velocity.y;

            //air depletes heat
            m_particles[a].heat -= m_heat_decay;

            //chat v5.1 for improved heat decay
            float h = m_particles[a].heat;

            if(m_pos_data[index1 + 1] < -0.95f) {
                h = 0.99f;
                m_particles[a].heat = h;
            }

            h = glm::clamp(h, 0.f, 1.f);
            glm::vec3 color;

            if(!settings.graded) {
                if(h < 0.33) {
                    float t = h/0.33;
                    color = glm::mix(glm::vec3{0,0,0}, glm::vec3{1,0,0}, t); //mix black -> red
                }
                else if (h < 0.66) {
                    float t = (h-0.33)/0.33f;
                    color = glm::mix(glm::vec3{1,0,0}, glm::vec3{1,0.5,0}, t); //red -> orange
                } else {
                    float t = (h - 0.66)/0.34f;
                    color = glm::mix(glm::vec3{1,0.5,0}, glm::vec3{1,0.9,0}, t); //orange -> almost yellow
                }
            }
            else {
                if (h < 0.25f) {
                    float t = h / 0.25f;
                    color = glm::mix(glm::vec3(0,0,0), glm::vec3(0,0,1), t);  // black → blue
                }
                else if (h < 0.50f) {
                    float t = (h - 0.25f) / 0.25f;
                    color = glm::mix(glm::vec3(0,0,1), glm::vec3(0,1,1), t);  // blue → cyan
                }
                else if (h < 0.75f) {
                    float t = (h - 0.50f) / 0.25f;
                    color = glm::mix(glm::vec3(0,1,1), glm::vec3(0,1,0), t);  // cyan → green
                }
                else {
                    float t = (h - 0.75f) / 0.25f;
                    color = glm::mix(glm::vec3(0,1,0), glm::vec3(1,0,0), t);  // green → red
                }
            }


            m_color_data[index1 + 0] = color.r;
            m_color_data[index1 + 1] = color.g;
            m_color_data[index1 + 2] = color.b;
        }
        m_particles[a].life -= m_decay;

    }

    //collisons
    for(int c = 0; c<m_collision_depth; ++c) {
        for (int a = 0; a<m_particles.size(); ++a) {
            int index1 = 3*a;

            for(int b = a+1; b<m_particles.size();++b) {
                int index2 = 3*b;

                glm::vec2 circle1{m_pos_data[index1 + 0], m_pos_data[index1 + 1]};
                glm::vec2 circle2{m_pos_data[index2 + 0], m_pos_data[index2 + 1]};
                if(checkOverlap(circle1, circle2, m_radius, m_radius)) {
                    //we have collision
                    glm::vec2 difference = circle1 - circle2;
                    float distance = glm::length(circle1 - circle2);
                    glm::vec3 normal = glm::vec3{glm::normalize(circle1 - circle2),0}; //normal formula for circle
                    float overlap = (distance - m_radius - m_radius)/2.f;

                    //displace circle1
                    m_pos_data[index1 + 0] -= overlap*(m_pos_data[index1+0]-m_pos_data[index2+0])/distance;
                    m_pos_data[index1 + 1] -= overlap*(m_pos_data[index1+1]-m_pos_data[index2+1])/distance;

                    //displace circle2
                    m_pos_data[index2 + 0] += overlap*(m_pos_data[index1+0]-m_pos_data[index2+0])/distance;
                    m_pos_data[index2 + 1] += overlap*(m_pos_data[index1+1]-m_pos_data[index2+1])/distance;

                    glm::vec3 relativeVelocity = m_particles[a].velocity - m_particles[b].velocity;
                    float velocityAboutNormal = glm::dot(relativeVelocity, normal);

                    if(velocityAboutNormal < 0) {

                        float impulseMagnitude = (-1.5)*velocityAboutNormal/2.f;

                        m_particles[a].velocity += impulseMagnitude*normal;
                        m_particles[b].velocity -= impulseMagnitude*normal;

                        //if vertical collsion, give particle a push
                        if(fabs(normal.y) > 0.8 && m_particles[a].life > 0.6) {
                            float rollDirection = (normal.x > 0) ? -1 : 1;
                            float rollStrength = 0.001f * fabs(normal.y);

                            m_pos_data[index1 + 0] += rollDirection*rollStrength;
                            m_pos_data[index2 + 0] -= rollDirection*rollStrength;
                        }
                    }

                    //chat v5.1 for improved heat transfer
                    m_particles[a].heat = glm::min(1.f, m_particles[a].heat + m_heat_transfer);
                    m_particles[b].heat = glm::min(1.f, m_particles[b].heat + m_heat_transfer);

                    //upward force due to heat; once reach threshold
                    if(m_particles[a].heat > 0.8f) {
                        m_particles[a].velocity.y += 0.0002*m_particles[a].heat;
                    }
                }
            }
        }
    }

    //side checks
    for (int a = 0; a<m_particles.size(); ++a) {
        int index1 = 3*a;
        //x
        if(m_pos_data[index1 + 0] < -m_side_bound + m_radius) {
            m_pos_data[index1 + 0] = -m_side_bound + m_radius;

            //horizontal bounce for recycling particles
            m_particles[a].heat = 0;
            m_particles[a].velocity.x = 0.001f;
            m_particles[a].velocity.y = -0.09f;
        }
        if(m_pos_data[index1 + 0] > m_side_bound - m_radius) {
            m_pos_data[index1 + 0] = m_side_bound - m_radius;

            //horizontal bounce for recycling particles
            m_particles[a].heat = 0;
            m_particles[a].velocity.x = -0.001f;
            m_particles[a].velocity.y = -0.09f;
        }
    }

    //bind, THEN "upload" offsets to vbos for shader
    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_pos_data.size()*sizeof(GLfloat), m_pos_data.data());

    glBindBuffer(GL_ARRAY_BUFFER, m_color_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_color_data.size()*sizeof(GLfloat), m_color_data.data());

}

void Realtime::initSkydome(){
    QImage img("/Users/skylerhall/Desktop/cs1230/proj-flameon/src/rogland_clear_night_2k.jpeg");
    if (img.isNull()) {
        std::cerr << "[Skydome] Failed to load sky texture image\n";
        m_skyTexture = 0;
        return;
    }

    QImage glImg = img.convertToFormat(QImage::Format_RGBA8888).mirrored();

    glGenTextures(1, &m_skyTexture);
    glBindTexture(GL_TEXTURE_2D, m_skyTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 glImg.width(), glImg.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, glImg.bits());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "[Skydome] Loaded sky texture " << m_skyTexture
              << " (" << glImg.width() << "x" << glImg.height() << ")\n";

}

void Realtime::drawSkydome(glm::vec3 camera_pos){
    glDepthMask(GL_FALSE);
    // See inside the sphere
    glDisable(GL_CULL_FACE);

    // Tell shader this is sky
    glUniform1i(is_sky_ID, 1);

    // Center dome on camera in world space
    float radius = 99.f; // large enough vs. your scene scale
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

void Realtime::paintGL() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_fbo_width, m_fbo_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view_mat = m_camera.getViewMatrix();

    glm::mat4 proj_mat = m_camera.getPerspectiveMatrix();

    glUseProgram(m_shader);

    // Need view, proj, and model in shader for mvp matrix
    glUniformMatrix4fv(view_ID, 1, GL_FALSE, &view_mat[0][0]);

    glm::vec3 camera_pos = glm::vec3(glm::inverse(view_mat)[3]);
    glUniform3f(camera_ID, camera_pos.x, camera_pos.y, camera_pos.z);

    //glm::mat4 proj_mat = m_camera.getPerspectiveMatrix();
    glUniformMatrix4fv(proj_ID, 1, GL_FALSE, &proj_mat[0][0]);


    // Phong Id's
    SceneGlobalData global = m_renderData.globalData;
    glUniform1f(ambient_k_ID, global.ka);
    glUniform1f(diffuse_k_ID, global.kd);
    glUniform1f(specular_k_ID, global.ks);

    // Fog uniforms
    glUniform1f(min_fog_ID, settings.fogMin);
    glUniform1f(max_fog_ID, m_fog);
    if(m_parsed)
    m_fog+=m_fog_rate;

    // Binding sky texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_skyTexture);
    GLint skyTexLoc = glGetUniformLocation(m_shader, "u_skyTex");
    glUniform1i(skyTexLoc, 0);
    drawSkydome(camera_pos);

    std::vector<RenderShapeData> &object_list = m_renderData.shapes;
    for (int k = 0; k < object_list.size(); k++) {
        RenderShapeData& object = object_list[k];

        glUniformMatrix4fv(model_ID, 1, GL_FALSE, &object.ctm[0][0]);

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
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(m_fire_shader);
    glBindVertexArray(m_fire_vao);

    glm::mat4 model = glm::mat4{1.f};

    glUniformMatrix4fv(glGetUniformLocation(m_fire_shader, "view_mat"), 1, GL_FALSE, &view_mat[0][0]);

    glUniformMatrix4fv(glGetUniformLocation(m_fire_shader, "proj_mat"), 1, GL_FALSE, &proj_mat[0][0]);

    glUniformMatrix4fv(glGetUniformLocation(m_fire_shader, "model_mat"), 1, GL_FALSE, &model[0][0]);

    glDepthMask(GL_FALSE);
    fireLoop();

    // draw triangles
    glVertexAttribDivisor(0,0);
    glVertexAttribDivisor(1,1);
    glVertexAttribDivisor(2,1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, m_vertexData.size()/3.f, m_particles.size());
    glDepthMask(GL_TRUE);

    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_default_fbo);
    glViewport(0, 0, m_screen_width, m_screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setBloom();
    //setKuwahara();
}

void Realtime::createShapes() {
    std::set<int> shape_exists;
    std::vector<RenderShapeData> &shapes = m_renderData.shapes;

    for (int k = 0; k < shapes.size(); k++) {
        m_parsed = true;
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

void Realtime::loadLUT() {
    // From chat for loading the file
    QFile qfile(":/resources/cool_tone.cube");
    if (!qfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open LUT file:" << qfile.errorString();
        return;
    }
    QTextStream in(&qfile);
    int lutSize = 64;
    std::vector<float> lutData;
    lutData.reserve(lutSize * lutSize * lutSize * 3);

    while (!in.atEnd()) {
        QString lineQ = in.readLine();
        std::string line = lineQ.toStdString();

        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("LUT_3D_SIZE", 0) == 0) continue;

        float r, g, b;
        if (sscanf(line.c_str(), "%f %f %f", &r, &g, &b) == 3) {
            lutData.push_back(r);
            lutData.push_back(g);
            lutData.push_back(b);
        }
    }

    // Load up the texture
    glGenTextures(1, &m_lut_texture);
    glBindTexture(GL_TEXTURE_3D, m_lut_texture);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, lutSize, lutSize, lutSize, 0, GL_RGB, GL_FLOAT, lutData.data());
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

    //Kuwahara filter
    GLuint kuwahara_ID = glGetUniformLocation(m_shader_bloom, "kuwaharaOn");
    glUniform1i(kuwahara_ID, settings.bloom); // or settings.bloom
    GLint texelLoc = glGetUniformLocation(m_shader_bloom, "u_texelSize");
    glUniform2f(texelLoc,
                1.0f / float(m_fbo_width),
                1.0f / float(m_fbo_height));

    GLuint grade_ID = glGetUniformLocation(m_shader_bloom, "graded");
    glUniform1i(grade_ID, settings.graded);
    GLuint LUT_ID = glGetUniformLocation(m_shader_bloom, "LUT");
    glUniform1i(LUT_ID, 2);

    glBindVertexArray(m_fullscreen_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_color_buffers[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_pingpong_color[!horizontal]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, m_lut_texture);

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
    //if (m_mouseDown) {
        // int posX = event->position().x();
        // int posY = event->position().y();
        // int deltaX = posX - m_prev_mouse_pos.x;
        // int deltaY = posY - m_prev_mouse_pos.y;
        // m_prev_mouse_pos = glm::vec2(posX, posY);

        // float amount = 0.01f;
        // SceneCameraData& cam = m_camera.camera;
        // glm::vec3 final_pos = cam.pos;

        // glm::vec3 look = glm::normalize(cam.look);
        // glm::vec3 up = glm::normalize(cam.up);

        // // Use deltaX and deltaY here to rotate
        // glm::mat3 rod_x = rodrigues(num * amount, up);
        // num+=2;
        // look = glm::normalize(rod_x * look);

        // glm::vec3 right = glm::normalize(glm::cross(look, up));
        // glm::mat3 rod_y = rodrigues(deltaY * amount, right);
        // look = glm::normalize(rod_y * look);

        // cam.look = glm::vec4(look, 0.f);

        // update(); // asks for a PaintGL() call to occur
    //}
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

float angle = 0.f;
void Realtime::timerEvent(QTimerEvent *event) {
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    float amount = 5.f * deltaTime;
    SceneCameraData& cam = m_camera.camera;
    glm::vec3 final_pos = cam.pos;

    glm::vec3 look = glm::normalize(cam.look);
    glm::vec3 up = glm::normalize(cam.up);
    glm::vec3 right = glm::normalize(glm::cross(look, up));

    float radius = 6.f;
    angle += 0.1f * deltaTime; //rotation speed here
    if(m_parsed) {
        final_pos += look * amount * -0.05f;
        cam.pos = glm::vec4(final_pos, 1.0f);
    }

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
