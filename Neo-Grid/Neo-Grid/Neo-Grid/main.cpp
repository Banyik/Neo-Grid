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
#include <algorithm>
#include <array>
#include <fstream>

int Triangle::globalID = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, TriangleFactory& triangleFactory);
std::string getFilePath();

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int tex_height;
int tex_width;
unsigned char* tex_data = nullptr;

glm::mat4 g_mvp;

float imageAspect;

bool isLoadImageButtonPressed = false;
bool isLoadFileButtonPressed = false;
unsigned int m_texture;


void loadImage(Shader cShader, GLFWwindow* window, TriangleFactory& triangleFactory);
void update_mvp(int width, int height);
void GetImgPixel(stbi_uc* image, size_t width, size_t x, size_t y, stbi_uc* r, stbi_uc* g, stbi_uc* b, stbi_uc* a);
void get_new_vertex_positions(float* out);
void new_grid_kernel(float x_max, float y_max, float size, std::vector<float>& hex_vertices, std::vector<int>& indices, std::vector<Hexagon>& hexagons, TriangleFactory& triangleFactory);
void create_new_grid(TriangleFactory& triangleFactory);
void load_hex_file(TriangleFactory& triangleFactory);
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
        if (isLoadFileButtonPressed) {
            load_hex_file(triangleFactory);
            isLoadFileButtonPressed = false;
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
    //std::cout << (int)width << " " << (int)height << std::endl;
    tex_height = height;
    tex_width = width;
    tex_data = image;
    //for (size_t i = 0; i < height; i++)
    //{
    //    for (size_t j = 0; j < width; j++)
    //    {
    //        GetImgPixel(image, width, i, j, &r, &g, &b, &a);
    //        //std::cout << i << " " << j << std::endl;
    //        //std::cout << (int)r << (int)g << (int)b << std::endl;
    //    }
    //}
    

    if (image == nullptr || tex_data == nullptr)
        //if (stbi_failure_reason())
            //std::cout << stbi_failure_reason() << std::endl;
        std::cout << "Cannot load texture" << std::endl;
    else {
        
        imageAspect = (float)width / height;
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    }

    //stbi_image_free(image);
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
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        isLoadFileButtonPressed = true;
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

void get_colors(std::array<float, 18> h_vertices, float x_max, float y_max, float px_x_step, float px_y_step, float* colors) {
    std::vector<int> x;
    std::vector<int> y;
    for (size_t v = 0; v < 18; v += 3)
    {
        x.push_back((int)((h_vertices[v] + x_max) / px_x_step));
        y.push_back((int)((h_vertices[v + 1] + y_max) / px_y_step));
    }
    int maxx = *max_element(x.begin(), x.end());
    int minx = *min_element(x.begin(), x.end());
    int maxy = *max_element(y.begin(), y.end());
    int miny = *min_element(y.begin(), y.end());

    double r = 0;
    double g = 0;
    double b = 0;
    int count = 0;
    int n = x.size();
    stbi_uc tr, tg, tb, ta;

    for (size_t ty = miny; ty < maxy; ty++)
    {
        for (size_t tx = minx; tx < maxx; tx++)
        {
            bool inside = false;
            for (int i = 0, j = n - 1; i < n; j = i++) {
                if (((y[i] > ty) != (y[j] > ty)) &&
                    (tx < (x[j] - x[i]) * (ty - y[i]) /
                        (y[j] - y[i]) + x[i])) {
                    inside = !inside;
                }
            }
            if (inside) {
                GetImgPixel(tex_data, tex_width, ty, tx, &tr, &tg, &tb, &ta);
                r += tr;
                g += tg;
                b += tb;
                count++;
            }
        }
    }

    r = (r / count)/255;
    g = (g / count)/255;
    b = (b / count)/255;
    colors[0] = r;
    colors[1] = g;
    colors[2] = b;
    colors[3] = r;
    colors[4] = g;
    colors[5] = b;
    colors[6] = r;
    colors[7] = g;
    colors[8] = b;
}

void create_hex_triangle(std::array<int, 12> h_indices, std::array<float, 18> h_vertices, float* colors, TriangleFactory& triangleFactory) {
    float t[9];
    for (size_t j = 0; j < 4; j++)
    {

        for (size_t k = 0; k < 3; k++)
        {
            int vi = h_indices[j * 3 + k];
            t[k * 3 + 0] = h_vertices[vi * 3 + 0];
            t[k * 3 + 1] = h_vertices[vi * 3 + 1];
            t[k * 3 + 2] = h_vertices[vi * 3 + 2];
        }
        Triangle t1(t, colors);
        triangleFactory.addTriangle(t1);
    }
}

void new_grid_kernel(float x_max, float y_max, float size, std::vector<float>& hex_vertices, std::vector<int>& indices, std::vector<Hexagon>& hexagons, TriangleFactory& triangleFactory) {

    float px_x_step = (x_max * 2) / (tex_width - 1);
    float px_y_step = (y_max * 2) / (tex_height - 1);

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

    float cx;
    float cy;
    float colors[9];
    std::array<float, FLOATS_PER_CELL> v;
    std::array<float, FLOATS_PER_CELL> prev_v;
    std::array<float, 18> h_v;
    std::array<int, 12> h_i;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size * 2; x++) {

            cx = -x_max + x * x_step;
            cy = y_max - y * y_step;

            v = {
                cx,             cy,               0.0f,
                cx + x_step,    cy,               0.0f,
                cx + x_step * 0.5f, cy - y_step * 0.25f, 0.0f,
                cx + x_step * 0.5f, cy - y_step * 0.5f, 0.0f,
                cx,             cy - y_step     * 0.75f, 0.0f,
                cx + x_step,    cy - y_step     * 0.75f, 0.0f,
                cx,             cy - y_step,      0.0f,
                cx + x_step,    cy - y_step,      0.0f
            };
            if (y > 0) {
                cx = -x_max + x * x_step;
                cy = y_max - (y - 1) * y_step;

                prev_v = {
                    cx,             cy,               0.0f,
                    cx + x_step,    cy,               0.0f,
                    cx + x_step * 0.5f, cy - y_step * 0.25f, 0.0f,
                    cx + x_step * 0.5f, cy - y_step * 0.5f, 0.0f,
                    cx,             cy - y_step * 0.75f, 0.0f,
                    cx + x_step,    cy - y_step * 0.75f, 0.0f,
                    cx,             cy - y_step,      0.0f,
                    cx + x_step,    cy - y_step,      0.0f
                };

                h_v = {
                    v[0], v[1], v[2],
                    v[3], v[4], v[5],
                    v[6], v[7], v[8],

                    prev_v[9], prev_v[10], prev_v[11],
                    prev_v[12], prev_v[13], prev_v[14],
                    prev_v[15], prev_v[16], prev_v[17],
                };
                h_i = {
                    0, 1, 2,
                    0, 3, 4,
                    1, 3, 5,
                    0, 1, 3
                };
                get_colors(h_v, x_max, y_max, px_x_step, px_y_step, colors);
                Hexagon h(x, y * 2 - 1, colors);
                hexagons.push_back(h);
                create_hex_triangle(h_i, h_v, colors, triangleFactory);
            }
            if (x > 0) {
                cx = -x_max + (x-1) * x_step;
                cy = y_max - y * y_step;

                prev_v = {
                    cx,             cy,               0.0f,
                    cx + x_step,    cy,               0.0f,
                    cx + x_step * 0.5f, cy - y_step * 0.25f, 0.0f,
                    cx + x_step * 0.5f, cy - y_step * 0.5f, 0.0f,
                    cx,             cy - y_step * 0.75f, 0.0f,
                    cx + x_step,    cy - y_step * 0.75f, 0.0f,
                    cx,             cy - y_step,      0.0f,
                    cx + x_step,    cy - y_step,      0.0f
                };

                h_v = {
                    v[0], v[1], v[2],
                    prev_v[6], prev_v[7], prev_v[8],
                    prev_v[9], prev_v[10], prev_v[11],

                    v[6],v[7],v[8],
                    v[9],v[10],v[11],
                    v[12], v[13], v[14]
                };
                h_i = {
                    0, 1, 2,
                    0, 2, 5,
                    0, 3, 4,
                    0, 4, 5
                };
                
                get_colors(h_v, x_max, y_max, px_x_step, px_y_step, colors);
                Hexagon h(x, y * 2, colors);
                hexagons.push_back(h);
                create_hex_triangle(h_i, h_v, colors, triangleFactory);
            }
            
            cell++;
        }
    }
    triangleFactory.finalizeBuffer();
    std::string final = "";
    for (size_t i = 0; i < hexagons.size(); i++)
    {
        double r = hexagons[i].getColors()[0];
        double g = hexagons[i].getColors()[1];
        double b = hexagons[i].getColors()[2];
        final += "{" + std::to_string(hexagons[i].getX()) + "," + std::to_string(hexagons[i].getY()) +
            ",(" + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + ")};";
    }
    std::ofstream hex_file("hexcells.hex");
    hex_file << final;
    hex_file.close();
}
void create_new_grid(TriangleFactory& triangleFactory) {
    float corner_vertices[12];
    get_new_vertex_positions(corner_vertices);
    const int size = 20;
    const int arr_size = size * (size * 2 * 24);
    std::vector<float> hex_vertices(arr_size);
    std::vector<int> indices(arr_size);
    std::vector<Hexagon> hexagons;
    new_grid_kernel(corner_vertices[0], corner_vertices[1], size, hex_vertices, indices, hexagons, triangleFactory);
    return;
}

void create_hex_cell(std::vector<Hexagon>& hexagons, TriangleFactory& triangleFactory) {
    float size = 0.0125f;

    float x_step = 0.75f;
    float y_step = 1.0f;
    float y_half_step = 0.5;
    
    float offset = 0.5;

    std::array<int, 12> indices = {
        0, 1, 2,
        1, 2, 3,
        2, 3, 4,
        3, 4, 5
    };

    for (size_t i = 0; i < hexagons.size(); i++)
    {
        int x = hexagons[i].getX();
        int y = hexagons[i].getY();

        float center_x = x * (2.0f * x_step);
        if (y % 2 != 0) {
            center_x += x_step;
        }
        
        float center_y = y * (-1.5f * y_step);
        
        std::array<float, 18> base_vertices = {
            (center_x)*size, (center_y)*size, 0.0f,
            (center_x - x_step) * size, (center_y - y_half_step) * size, 0.0f,
            (center_x + x_step) * size, (center_y - y_half_step) * size, 0.0f,
            (center_x - x_step) * size, (center_y - 1.5f * y_step) * size, 0.0f,
            (center_x + x_step) * size, (center_y - 1.5f * y_step) * size, 0.0f,
            (center_x)*size, (center_y - 2.0f * y_step) * size, 0.0f
        };
        create_hex_triangle(indices, base_vertices, hexagons[i].getColors(), triangleFactory);
    }
    triangleFactory.finalizeBuffer();
}

void load_hex_file(TriangleFactory& triangleFactory) {
    std::string filename = getFilePath();
    if (filename == "") {
        return;
    }
    std::string file_content;
    std::string final_file_content;
    std::ifstream hex_file(filename);
    while (std::getline(hex_file, file_content)) {
        final_file_content += file_content;
    }
    hex_file.close();
    std::string x, y, r, g, b;
    int state = 0;
    std::vector<Hexagon> hexagons;
    for (size_t i = 0; i < final_file_content.size(); i++)
    {
        switch (state)
        {
        case 0:
            if (final_file_content[i] == '{'){
                state = 1;
            }
            break;
        case 1:
            if (final_file_content[i] == ',') {
                state = 2;
            }
            else {
                x += final_file_content[i];
            }
            break;
        case 2:
            if (final_file_content[i] == ',') {
                state = 3;
            }
            else {
                y += final_file_content[i];
            }
            break;
        case 3:
            if (final_file_content[i] == '(') {
                state = 4;
            }
            break;
        case 4:
            if (final_file_content[i] == ',') {
                state = 5;
            }
            else {
                r += final_file_content[i];
            }
            break;
        case 5:
            if (final_file_content[i] == ',') {
                state = 6;
            }
            else {
                g += final_file_content[i];
            }
            break;
        case 6:
            if (final_file_content[i] == ')') {
                state = 7;
            }
            else {
                b += final_file_content[i];
            }
            break;
        case 7:
            state = 0;
            float colors[9] = {
                std::stof(r),std::stof(g), std::stof(b),
                std::stof(r),std::stof(g), std::stof(b),
                std::stof(r),std::stof(g), std::stof(b)
            };
            int xin = std::stoi(x);
            int yin = std::stoi(y);
            Hexagon h(xin, yin, colors);
            hexagons.push_back(h);
            x.clear();
            y.clear();
            r.clear();
            g.clear();
            b.clear();
            break;
        }
    }

    create_hex_cell(hexagons, triangleFactory);
}