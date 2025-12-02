#include "cylinder.h"

std::vector<float> Cylinder::generateShape() {
    return m_vertexData;
}


void Cylinder::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = std::max(param1, 1);
    m_param2 = std::max(param2, 3);
    setVertexData();
}


void Cylinder::setVertexData() {
    float increment = glm::radians(360.f / m_param2);

    for (int i = 0; i < m_param2; i++) {
        float theta1 = i * increment;
        float theta2 = (i + 1) * increment;
        makeWedge(theta1, theta2);
    }
}


void Cylinder::makeWedge(float currentTheta, float nextTheta) {
    // Bottom cap
    makeCapSlice(currentTheta, nextTheta, -0.5f, glm::vec3(0, -1.0f, 0.0f));
    // Top Cap
    makeCapSlice(currentTheta, nextTheta, 0.5f, glm::vec3(0, 1.0f, 0.0f));
    // Body
    makeSlopeSlice(currentTheta, nextTheta);
}


void Cylinder::makeSlopeSlice(float currentTheta, float nextTheta){
    float incrementY = 1.0f / float(m_param1);

    for (int i = 0; i < m_param1; i++) {
        float topY = 0.5f - i * incrementY;
        float bottomY = 0.5f - (i + 1) * incrementY;

        glm::vec3 topLeft = glm::vec3(0.5f * cos(currentTheta), topY, 0.5f * sin(currentTheta));
        glm::vec3 topRight = glm::vec3(0.5f * cos(nextTheta), topY, 0.5f * sin(nextTheta));
        glm::vec3 bottomLeft = glm::vec3(0.5f * cos(currentTheta), bottomY, 0.5f * sin(currentTheta));
        glm::vec3 bottomRight = glm::vec3(0.5 * cos(nextTheta), bottomY, 0.5f * sin(nextTheta));

        makeSlopeTile(topLeft, topRight, bottomLeft, bottomRight);
    }
}


void Cylinder::makeSlopeTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight) {
    glm::vec3 normTL = calcNorm(topLeft);
    glm::vec3 normTR = calcNorm(topRight);
    glm::vec3 normBL = calcNorm(bottomLeft);
    glm::vec3 normBR = calcNorm(bottomRight);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normTL);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normBR);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normBL);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normTL);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normTR);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normBR);
}


glm::vec3 Cylinder::calcNorm(glm::vec3& pt) {
    return glm::normalize(glm::vec3(pt.x, 0, pt.z));
}


void Cylinder::makeCapSlice(float currentTheta, float nextTheta, float y, glm::vec3 normal) {
    float increment = 0.5f / float(m_param1);

    for (int i = 0; i < m_param1; i++) {
        float val1 = i * increment;
        float val2 = (i + 1) * increment;

        glm::vec3 topLeft = glm::vec3(val1 * cos(currentTheta), y, val1 * sin(currentTheta));
        glm::vec3 topRight = glm::vec3(val1 * cos(nextTheta), y, val1 * sin(nextTheta));
        glm::vec3 bottomLeft = glm::vec3(val2 * cos(currentTheta), y, val2 * sin(currentTheta));
        glm::vec3 bottomRight = glm::vec3(val2 * cos(nextTheta), y, val2 * sin(nextTheta));

        if (y < 0.0f) {
            // Bottom cap
            makeCapTile(topLeft, topRight, bottomLeft, bottomRight, normal);
        } else {
            // Top cap --> Need to flip inputs to make it CCW
            makeCapTile(topRight, topLeft, bottomRight, bottomLeft, normal);
        }
    }
}


void Cylinder::makeCapTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight, glm::vec3 normal) {
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normal);
}

void Cylinder::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
