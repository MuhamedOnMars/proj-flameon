#include <stdexcept>
#include <iostream>
#include "camera.h"
#include "settings.h"

glm::mat4 Camera::getViewMatrix() const {
    // Get u,v,w
    glm::vec4 w = -glm::normalize(camera.look);
    glm::vec4 v = glm::normalize(camera.up - glm::dot(camera.up, w) * w);
    glm::vec3 u_temp = glm::cross(glm::vec3(v), glm::vec3(w));
    glm::vec4 u = glm::vec4(u_temp, 0.0f);

    glm::vec4 r_col0 = glm::vec4(u[0], v[0], w[0], 0);
    glm::vec4 r_col1 = glm::vec4(u[1], v[1], w[1], 0);
    glm::vec4 r_col2 = glm::vec4(u[2], v[2], w[2], 0);
    glm::vec4 r_col3 = glm::vec4(0, 0, 0, 1);
    glm::mat4 rot_mat = glm::mat4(r_col0, r_col1, r_col2, r_col3);

    glm::vec4 t_col0 = glm::vec4(1, 0, 0, 0);
    glm::vec4 t_col1 = glm::vec4(0, 1, 0, 0);
    glm::vec4 t_col2 = glm::vec4(0, 0, 1, 0);
    glm::vec4 t_col3 = glm::vec4(-camera.pos[0], -camera.pos[1], -camera.pos[2], 1);
    glm::mat4 trans_mat = glm::mat4(t_col0, t_col1, t_col2, t_col3);

    return rot_mat * trans_mat;
}

glm::mat4 Camera::getPerspectiveMatrix() const {
    float near = settings.nearPlane;
    float far = settings.farPlane;
    float c = -near/far;

    glm::mat4 mapping_mat(1.0f);
    mapping_mat[2][2] = -2.0f;
    mapping_mat[3][2] = -1.0f;

    glm::mat4 unhinge_mat(1.0f);
    unhinge_mat[2][2] = 1.0f/(1.0f + c);
    unhinge_mat[3][2] = -c/(1.0f + c);
    unhinge_mat[2][3] = -1.0f;
    unhinge_mat[3][3] = 0.0f;

    glm::mat4 scale_mat(1.0f);
    scale_mat[0][0] = 1.0 / (far * getAspectRatio() * tan(getHeightAngle()/2.0f));
    scale_mat[1][1] = 1.0f / (far * tan(getHeightAngle()/2.0f));
    scale_mat[2][2] = 1.0f / far;

    return mapping_mat * unhinge_mat * scale_mat;
}

float Camera::getAspectRatio() const {
    return float(width) / float(height);
}

float Camera::getHeightAngle() const {
    return camera.heightAngle;
}

/*float Camera::getFocalLength() const {
    // Optional TODO: implement the getter or make your own design
    throw std::runtime_error("not implemented");
}

float Camera::getAperture() const {
    // Optional TODO: implement the getter or make your own design
    throw std::runtime_error("not implemented");
}*/
