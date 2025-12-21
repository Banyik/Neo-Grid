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
#include <cmath>
#include <vector>

#include "hexagon.h"

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
void new_grid_kernel(float x_max, float y_max, float size, float* hex_vertices, int* indices, std::vector<Hexagon>& hexagons, TriangleFactory& triangleFactory);
void generate_new_grid_vertices(float xmax, float xmin, float ymax, float ymin, int size, float* hex_vertices, int* indices);
void create_new_grid(TriangleFactory& triangleFactory);
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
        create_new_grid(triangleFactory);
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
    {  1.0f,  1.0f, 0.0f, 1.0f }, // top right
    {  1.0f, -1.0f, 0.0f, 1.0f }, // bottom right
    { -1.0f, -1.0f, 0.0f, 1.0f }, // bottom left
    { -1.0f,  1.0f, 0.0f, 1.0f }  // top left 
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

void new_grid_kernel(float x_max, float y_max, float size, float* hex_vertices, int* indices, std::vector<Hexagon>& hexagons, TriangleFactory& triangleFactory) {
    const int VERTS_PER_CELL = 8;
    const int FLOATS_PER_VERT = 3;
    const int FLOATS_PER_CELL = VERTS_PER_CELL * FLOATS_PER_VERT;
    const int INDICES_PER_CELL = 24;

    float x_step = (2.0f * x_max) / (size * 2);
    float y_step = (2.0f * y_max) / size;

    const int kernel_indices[24] = {
        0,1,2,  0,2,4,
        1,2,5,  2,3,4,
        2,3,5,  3,4,6,
        3,5,7,  3,6,7
    };

    const int upper_hex_coordinates[3] = {
        0, 1, 2
    };
    const int lower_hex_coordinates[5] = {
        3, 4, 5, 6, 7
    };

    const int left_hex_coordinates[4] = {
        0, 2, 3, 4
    };

    const int right_hex_coordinates[4] = {
        1, 2, 3, 5
    };

    int cell = 0;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size * 2; x++) {

            float cx = -x_max + x * x_step;
            float cy = y_max - y * y_step;

            float v[FLOATS_PER_CELL] = {
                cx,             cy,               0.0f,
                cx + x_step,    cy,               0.0f,
                cx + x_step * 0.5f, cy - y_step * 0.25f, 0.0f,
                cx + x_step * 0.5f, cy - y_step * 0.5f, 0.0f,
                cx,             cy - y_step     * 0.75f, 0.0f,
                cx + x_step,    cy - y_step     * 0.75f, 0.0f,
                cx,             cy - y_step,      0.0f,
                cx + x_step,    cy - y_step,      0.0f
            };

           /* int v_offset = cell * FLOATS_PER_CELL;
            int i_offset = cell * INDICES_PER_CELL;
            int base_vert = cell * VERTS_PER_CELL;

            for (int i = 0; i < FLOATS_PER_CELL; i++)
                hex_vertices[v_offset + i] = v[i];

            for (int i = 0; i < INDICES_PER_CELL; i++)
                indices[i_offset + i] = kernel_indices[i] + base_vert;
                */
            if (x > 0) {
                float cx = -x_max + (x-1) * x_step;
                float cy = y_max - y * y_step;

                float prev_v[FLOATS_PER_CELL] = {
                    cx,             cy,               0.0f,
                    cx + x_step,    cy,               0.0f,
                    cx + x_step * 0.5f, cy - y_step * 0.25f, 0.0f,
                    cx + x_step * 0.5f, cy - y_step * 0.5f, 0.0f,
                    cx,             cy - y_step * 0.75f, 0.0f,
                    cx + x_step,    cy - y_step * 0.75f, 0.0f,
                    cx,             cy - y_step,      0.0f,
                    cx + x_step,    cy - y_step,      0.0f
                };

                float h_v[18] = {
                    v[0], v[1], v[2],
                    prev_v[6], prev_v[7], prev_v[8],
                    prev_v[9], prev_v[10], prev_v[11],

                    v[6],v[7],v[8],
                    v[9],v[10],v[11],
                    v[12], v[13], v[14]
                };
                int h_i[12] = {
                    0, 1, 2,
                    0, 2, 5,
                    0, 3, 4,
                    0, 4, 5
                };
                Hexagon h(h_v, h_i);
                hexagons.push_back(h);
            }
            if (y > 0) {
                float cx = -x_max + x * x_step;
                float cy = y_max - (y-1) * y_step;

                float prev_v[FLOATS_PER_CELL] = {
                    cx,             cy,               0.0f,
                    cx + x_step,    cy,               0.0f,
                    cx + x_step * 0.5f, cy - y_step * 0.25f, 0.0f,
                    cx + x_step * 0.5f, cy - y_step * 0.5f, 0.0f,
                    cx,             cy - y_step * 0.75f, 0.0f,
                    cx + x_step,    cy - y_step * 0.75f, 0.0f,
                    cx,             cy - y_step,      0.0f,
                    cx + x_step,    cy - y_step,      0.0f
                };

                float h_v[18] = {
                    v[0], v[1], v[2],
                    v[3], v[4], v[5],
                    v[6], v[7], v[8],

                    prev_v[9], prev_v[10], prev_v[11],
                    prev_v[12], prev_v[13], prev_v[14],
                    prev_v[15], prev_v[16], prev_v[17],
                };
                int h_i[12] = {
                    0, 1, 2,
                    0, 3, 4,
                    1, 3, 5,
                    0, 1, 3
                };
                Hexagon h(h_v, h_i);
                hexagons.push_back(h);
            }
            cell++;
        }
    }

    for (size_t i = 0; i < hexagons.size(); i++)
    {
        int* h_indices = hexagons[i].getIndices();
        float* h_vertices = hexagons[i].getVertices();
        float t[9];
        for (size_t j = 0; j < 4; j++)
        {
            for (size_t k = 0; k < 3; k++)
            {
                int vi = hexagons[i].getIndices()[j * 3 + k];
                t[k * 3 + 0] = hexagons[i].getVertices()[vi * 3 + 0];
                t[k * 3 + 1] = hexagons[i].getVertices()[vi * 3 + 1];
                t[k * 3 + 2] = hexagons[i].getVertices()[vi * 3 + 2];
            }

            float r = 0.0075f * (i + 1);
            float g = 0.1f;
            float b = 0.03f;
            float colors[] = {
                r, g, b,
                r, g, b,
                r, g, b
            };

            Triangle t1(t, colors);
            triangleFactory.addTriangle(t1);

        }
    } 
}

/*
min 84 hex vertices
min 120 hex indices
*/
void generate_new_grid_vertices(float xmax, float xmin, float ymax, float ymin, int size, float *hex_vertices, int *indices) {
    float o_xmax = xmax * 2;
    float o_ymax = ymax * 2;
    float o_xmin = 0.0;
    float o_ymin = 0.0;

    //float hex_vertices[84];
    int h_size = 5 * size;
    int d_size_x1 = 5 * size;
    int d_size_x2 = 6 * size;
    int d_size_y1 = 4 * size;

    int indexer = 0;

    for (size_t i = 0; i < h_size; i++)
    {

        if (i % 2 == 0) {
            for (size_t j = 0; j < d_size_x2; j++)
            {
                //std::cout << "I:" << i << "| X: " << (j * (o_xmax / d_size_x1)) - xmax << " Y : " <<((4-i)*(o_ymax/4)) - ymax << std::endl;
                hex_vertices[indexer++] = (j * (o_xmax / d_size_x1)) - xmax;
                hex_vertices[indexer++] = ((4 - i) * (o_ymax / 4)) - ymax;
                hex_vertices[indexer++] = 0.0f;
            }
        }
        else {
            for (size_t j = 0; j < d_size_x1; j++)
            {
                //std::cout << "I:" << i << "| X: " << ((j + 1) * (o_xmax / d_size_x2)) - xmax << " Y: " << ((4-i) * (o_ymax / 4)) - ymax << std::endl;

                hex_vertices[indexer++] = ((j+1) * (o_xmax / d_size_x2)) - xmax;
                hex_vertices[indexer++] = ((4 - i) * (o_ymax / 4)) - ymax;
                hex_vertices[indexer++] = 0.0f;
            }
        }
    }

    int h_hex_h_count = 3 * size;
    int v_hex_h_count = 4 * size;
    int startx[9] = { 0, 1, 6, 
                      1, 6, 7,
                      1, 2, 7};
    int starty[9] = { 2, 7, 8,
                      2, 3, 8,
                      3, 8, 9};

    int startz1[3] = { 0, d_size_x1 + 1, d_size_x1 + d_size_x2 };
    int startz2[3] = { d_size_x1, d_size_x1  + d_size_x1, d_size_x1 + d_size_x2 + d_size_x1};

    //int indices[120];
    int indices_indexer = 0;
    int xindexer = 0;
    int yindexer = 2;
    for (size_t i = 0; i < v_hex_h_count; i++)
    {
        for (size_t j = 0; j < h_hex_h_count; j++)
        {
            if ((i + j) % 2 == 0) {
                for (size_t k = 0; k < 9; k++)
                {
                    indices[indices_indexer] = startx[k];
                    indices_indexer++;
                    if (xindexer == 0) {
                        startx[k] += 3;
                    }
                    else {
                        startx[k] += 4;
                    }
                }
                xindexer++;
                if (xindexer >= 3) {
                    xindexer = 0;
                }
            }
            else {
                for (size_t k = 0; k < 9; k++)
                {
                    indices[indices_indexer] = starty[k];
                    indices_indexer++;
                    if (yindexer == 0) {
                        starty[k] += 3;
                    }
                    else {
                        starty[k] += 4;
                    }
                }
                yindexer++;
                if (yindexer >= 3) {
                    yindexer = 0;
                }
            }
        }
    }
    for (size_t i = 0; i < v_hex_h_count; i++)
    {
        if (i % 2 == 0) {
            for (size_t k = 0; k < 3; k++)
            {
                indices[indices_indexer] = startz1[k];
                startz1[k] += d_size_x1 + d_size_x2;
                indices_indexer++;
            }
        }
        else {
            for (size_t k = 0; k < 3; k++)
            {
                indices[indices_indexer] = startz2[k];
                startz2[k] += d_size_x1 + d_size_x2;
                indices_indexer++;
            }
        }
    }
}

void create_new_grid(TriangleFactory& triangleFactory) {
    float corner_vertices[12];
    get_new_vertex_positions(corner_vertices);
    const int size = 10;
    const int arr_size = size * (size * 2 * 24);
    float hex_vertices[arr_size];
    int indices[arr_size];
    std::vector<Hexagon> hexagons;
    new_grid_kernel(corner_vertices[0], corner_vertices[1], size, hex_vertices, indices, hexagons, triangleFactory);
    return;
    int i = 0;
    int indices_idx = 0;
    while (i < arr_size/3)
    {
        float t[9];
        int idx = 0;
        for (size_t j = 0; j < 3; j++)
        {
            for (size_t k = 0; k < 3; k++)
            {
                t[idx++] = hex_vertices[(indices[indices_idx] * 3) + k];
            }
            std::cout << indices[indices_idx]*3 << std::endl;

            indices_idx++;
            /*std::cout << i * j << std::endl;
            for (size_t k = 0; k < 3; k++)
            {
                t[idx] = hex_vertices[curr_idx + idx];
                std::cout << curr_idx + idx;
                idx++;
            }
            //std::cout << std::endl;*/
        }
        std::cout << "---" << std::endl;

        float r = 0.03f*(i+1);
        float g = 0.1f;
        float b = 0.03f;
        float colors[] = {
            r, g, b,
            r, g, b,
            r, g, b
        };
        Triangle t1(t, colors);
        triangleFactory.addTriangle(t1);
        i++;
    }
    std::cout << "---------" << std::endl;
    

    return;
    float grid_vertices[30] = {
    corner_vertices[0], corner_vertices[1], 0.0f, //top right
    corner_vertices[0], corner_vertices[1] / 3, 0.0f,
    corner_vertices[3], corner_vertices[4], 0.0f, // bottom right
    corner_vertices[3]/3, corner_vertices[4], 0.0f,
    corner_vertices[6], corner_vertices[7], 0.0f, // bottom left
    corner_vertices[9], corner_vertices[7]/3, 0.0f,
    corner_vertices[9], corner_vertices[10], 0.0f, // top left 
    
    corner_vertices[9]/3, corner_vertices[1], 0.0f,

    corner_vertices[9] / 3, corner_vertices[7] / 3, 0.0f,
    corner_vertices[3] / 3, corner_vertices[1] / 3, 0.0f,
    };
    /*float r = 0.23f;
    float g = 0.23f;
    float b = 0.23f;
    float colors[] = {
        r, g, b,
        r, g, b,
        r, g, b
    };

    float hex1_0[9] = {
        grid_vertices[21], grid_vertices[22], grid_vertices[23],
        grid_vertices[27], grid_vertices[28], grid_vertices[29],
        grid_vertices[18], grid_vertices[19], grid_vertices[20],
    };

    float hex1_1[9] = {
        grid_vertices[18], grid_vertices[19], grid_vertices[20],
        grid_vertices[15], grid_vertices[16], grid_vertices[17],
        grid_vertices[24], grid_vertices[25], grid_vertices[26],
    };
    Triangle t1(hex1_0, colors);
    Triangle t2(hex1_1, colors);
    triangleFactory.addTriangle(t1);
    triangleFactory.addTriangle(t2);*/
}