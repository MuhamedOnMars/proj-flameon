#include "sphere.h"

std::vector<float> Sphere::generateShape() {
    return m_vertexData;
}


void Sphere::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = std::max(param1, 2);
    m_param2 = std::max(param2, 3);
    setVertexData();
}


void Sphere::setVertexData() {
    makeSphere();
}


void Sphere::makeSphere() {
    float increment = glm::radians(360.f / m_param2);

    for (int i = 0; i < m_param2; i++) {
        float theta1 = i * increment;
        float theta2 = (i + 1) * increment;
        makeWedge(theta1, theta2);
    }
}


void Sphere::makeWedge(float currentTheta, float nextTheta) {
    float increment = glm::radians(180.f / m_param1);

    for (int i = 0; i < m_param1; i++) {
        float phi1 = i * increment;
        float phi2 = (i + 1) * increment;

        glm::vec3 topLeft = glm::vec3(0.5f * sin(phi1) * cos(currentTheta),
                                      0.5f * cos(phi1),
                                     -0.5f * sin(phi1) * sin(currentTheta));
        glm::vec3 topRight = glm::vec3(0.5f * sin(phi1) * cos(nextTheta),
                                       0.5f * cos(phi1),
                                      -0.5f * sin(phi1) * sin(nextTheta));
        glm::vec3 bottomLeft = glm::vec3(0.5f * sin(phi2) * cos(currentTheta),
                                         0.5f * cos(phi2),
                                        -0.5f * sin(phi2) * sin(currentTheta));
        glm::vec3 bottomRight = glm::vec3(0.5f * sin(phi2) * cos(nextTheta),
                                          0.5f * cos(phi2),
                                         -0.5f * sin(phi2) * sin(nextTheta));

        makeTile(topLeft, topRight, bottomLeft, bottomRight);
    }
}


void Sphere::makeTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight) {
    glm::vec3 normTL = glm::normalize(topLeft);
    glm::vec3 normTR = glm::normalize(topRight);
    glm::vec3 normBL = glm::normalize(bottomLeft);
    glm::vec3 normBR = glm::normalize(bottomRight);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normTL);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normBL);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normBR);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normTL);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normBR);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normTR);
}

void Sphere::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
