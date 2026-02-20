#include "triangle_factory.h"
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 
#include <iostream>
#include "shader_s.h"
#define STB_IMAGE_IMPLEMENTATION

class TriangleFactory {
private:
    std::vector<Triangle> triangles;
    std::vector<float> vertexData;
    GLuint VAO, VBO;


    void rebuildVertexBuffer() {
        vertexData.clear();
        for (auto t : triangles) {
            for (int i = 0; i < 3; ++i) {
                vertexData.push_back(t.getVertices()[i * 3 + 0]);
                vertexData.push_back(t.getVertices()[i * 3 + 1]);
                vertexData.push_back(t.getVertices()[i * 3 + 2]);
                vertexData.push_back(t.getColors()[i * 3 + 0]);
                vertexData.push_back(t.getColors()[i * 3 + 1]);
                vertexData.push_back(t.getColors()[i * 3 + 2]);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
    }
public:
    void clearTriangles() {
        triangles.clear();
        rebuildVertexBuffer();
    }
    TriangleFactory() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void addTriangle(const Triangle& tri) {
        triangles.push_back(tri);
    }
    void finalizeBuffer() {
        rebuildVertexBuffer();
    }

    void draw(Shader& shader) {
        if (vertexData.empty()) return;
        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 6);
    }
};