#include "sceneparser.h"
#include "scenefilereader.h"
#include "shape/objloader.h"
#include "shape/sphere.h"
#include "shape/cube.h"
#include "shape/cone.h"
#include "shape/cylinder.h"
#include "settings.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

void SceneParser::dfsData(glm::mat4 total_ctm, SceneNode &curr_node, RenderData &renderData) {
    // Combine new transformations into the ctm
    std::vector<SceneTransformation*> transforms = curr_node.transformations;
    for (int j = 0; j < transforms.size(); j++) {
        SceneTransformation transform = *transforms[j];
        glm::mat4 new_mat;

        // Check for type of transform
        if (transform.type == TransformationType::TRANSFORMATION_TRANSLATE) {
            new_mat = glm::translate(glm::mat4(1.0f), transform.translate);
        } else if (transform.type == TransformationType::TRANSFORMATION_SCALE) {
            new_mat = glm::scale(glm::mat4(1.0f), transform.scale);
        } else if (transform.type == TransformationType::TRANSFORMATION_ROTATE) {
            new_mat = glm::rotate(glm::mat4(1.0f), transform.angle, transform.rotate);
        } else {
            new_mat = transform.matrix;
        }

        total_ctm = total_ctm * new_mat;
    }

    // Add primitives and lights to be used for rendering
    std::vector<ScenePrimitive*> curr_prims = curr_node.primitives;
    for (int i = 0; i < curr_prims.size(); i++) {
        RenderShapeData new_prim = {*curr_prims[i], total_ctm};
        if (new_prim.primitive.type == PrimitiveType::PRIMITIVE_SPHERE) {
            new_prim.shape = new Sphere();
        }
        else if (new_prim.primitive.type == PrimitiveType::PRIMITIVE_CYLINDER) {
            new_prim.shape = new Cylinder();
        }
        else if (new_prim.primitive.type == PrimitiveType::PRIMITIVE_CONE) {
            new_prim.shape = new Cone();
        }
        else if (new_prim.primitive.type == PrimitiveType::PRIMITIVE_CUBE) {
            new_prim.shape = new Cube();
        }
        else if (new_prim.primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            new_prim.shape = new ObjLoader(new_prim.primitive.meshfile);
        }

        renderData.shapes.push_back(new_prim);
    }

    std::vector<SceneLight*> curr_lights = curr_node.lights;
    for (int l = 0; l < curr_lights.size(); l++) {
        SceneLight light = *curr_lights[l];

        // Needs to be in world space
        glm::vec4 pos = total_ctm * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 dir = glm::normalize(total_ctm * glm::vec4(light.dir.x, light.dir.y, light.dir.z, 0.0f));

        SceneLightData new_light = {light.id, light.type, light.color, light.function, pos, dir,
                                    light.penumbra, light.angle, light.width, light.height};
        renderData.lights.push_back(new_light);
    }

    // Must get the children's properties as well
    std::vector<SceneNode*> children = curr_node.children;
    for (int k = 0; k < children.size(); k++) {
        dfsData(total_ctm, *children[k], renderData);
    }
}

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();

    SceneNode* root = fileReader.getRootNode();
    renderData.shapes.clear();
    glm::mat4 total_ctm = glm::mat4(1.0f);

    dfsData(total_ctm, *root, renderData);

    return true;
}
