#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include "shader_s.h"
#include <windows.h>
#include <commdlg.h>


#include "triangle.h"
#include "triangle_factory.cpp"

int Triangle::globalID = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, TriangleFactory& triangleFactory);
std::string getFilePath();

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
glm::mat4 g_mvp;

float imageAspect;

bool isLoadImageButtonPressed = false;
unsigned int m_texture;


void loadImage(Shader cShader, GLFWwindow* window, TriangleFactory& triangleFactory);
void update_mvp(int width, int height);
void GetImgPixel(stbi_uc* image, size_t width, size_t x, size_t y, stbi_uc* r, stbi_uc* g, stbi_uc* b, stbi_uc* a);
void get_new_vertex_positions(float* out);
float vertices[] = {
    // positions          // colors           // texture coords
     1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
     1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
    -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
    -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
};
unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
};

int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Neo-Grid", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    update_mvp(SCR_WIDTH, SCR_HEIGHT);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    TriangleFactory triangleFactory;

    Shader cShader("shader.vs", "shader.fs");
    Shader basicShader("basic_shader.vs", "basic_shader.fs");

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
 

    while (!glfwWindowShouldClose(window))
    {
        if (isLoadImageButtonPressed) {
            loadImage(cShader, window, triangleFactory);
            isLoadImageButtonPressed = false;
        }
        processInput(window, triangleFactory);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);

        cShader.use();
        GLuint mvpLoc = glGetUniformLocation(cShader.ID, "mvp");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(g_mvp));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        triangleFactory.draw(basicShader);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
    return 0;
}
void loadImage(Shader cShader, GLFWwindow* window, TriangleFactory& triangleFactory) {
    triangleFactory.clearTriangles();
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
    }
    glGenTextures(1, &m_texture);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, comp;
    stbi_set_flip_vertically_on_load(true);
    std::string filename = getFilePath();
    if (filename == "") {
        return;
    }
    unsigned char* image = stbi_load(filename.c_str(), &width, &height, &comp, STBI_rgb_alpha);
    stbi_uc r, g, b, a;
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            GetImgPixel(image, width, i, j, &r, &g, &b, &a);
            //std::cout << i << " " << j << std::endl;
            //std::cout << (int)r << (int)g << (int)b << std::endl;
        }
    }
    

    if (image == nullptr)
        //if (stbi_failure_reason())
            //std::cout << stbi_failure_reason() << std::endl;
        std::cout << "Cannot load texture" << std::endl;
    else {
        
        imageAspect = (float)width / height;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(image);
    cShader.use();
    cShader.setInt("m_texture", 0);

    int curwidth, curheight;
    glfwGetWindowSize(window, &curwidth, &curheight);
    update_mvp(curwidth, curheight);
}
void GetImgPixel(stbi_uc* image, size_t width, size_t x, size_t y, stbi_uc* r, stbi_uc* g, stbi_uc* b, stbi_uc* a) {
    const int p = (4 * (x * width + y));
    *r = image[p + 0];
    *g = image[p + 1];
    *b = image[p + 2];
    *a = image[p + 3];
}
std::string getFilePath() {
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "All Files\0*.*\0JPEG Image\0*.jpg;*.jpeg\0PNG Image\0*.png\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
    {
        return std::string(ofn.lpstrFile);
    }

    return "";
}

void processInput(GLFWwindow* window, TriangleFactory& triangleFactory)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        isLoadImageButtonPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        float nv[12];
        get_new_vertex_positions(nv);
        float new_new_vert[] = {
            nv[0], nv[1], nv[2],
            //nv[3], nv[4], nv[5],
            nv[6], nv[7], nv[8],
            nv[9], nv[10], nv[11]
        };

        float r = 0.23f;
        float g = 0.23f;
        float b = 0.23f;
        float colors[] = {
            r, g, b,
            r, g, b,
            r, g, b
        };
        Triangle t1(new_new_vert, colors);
        triangleFactory.addTriangle(t1);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    update_mvp(width, height);
}

void update_mvp(int width, int height)
{
    float windowAspect = (float)width / (float)height;
    glm::mat4 projection = glm::ortho(-windowAspect, windowAspect, -1.0f, 1.0f, -1.0f, 1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);

    if (imageAspect > 1.0f) {
        model = glm::scale(model, glm::vec3(1.0f, 1.0f / imageAspect, 1.0f));
    }
    else {
        model = glm::scale(model, glm::vec3(imageAspect, 1.0f, 1.0f));
    }
    
    g_mvp = projection * view * model;
}

void get_new_vertex_positions(float* out) {
    glm::vec4 original_vertices[4] = {
    {  1.0f,  1.0f, 0.0f, 1.0f },
    {  1.0f, -1.0f, 0.0f, 1.0f },
    { -1.0f, -1.0f, 0.0f, 1.0f },
    { -1.0f,  1.0f, 0.0f, 1.0f } 
    };

    int k = 0;
    for (int i = 0; i < 4; ++i) {
        glm::vec4 transformed = g_mvp * original_vertices[i];
        for (size_t j = 0; j < 3; j++)
        {
            out[k++] = transformed[j];
        }
    }
}