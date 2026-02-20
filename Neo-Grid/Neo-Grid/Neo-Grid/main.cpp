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
#include "tnonagon.h"

#include "triangle.h"
#include "triangle_factory.cpp"
#include <algorithm>
#include <array>
#include <fstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int Triangle::globalID = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
std::string getFilePath();

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int tex_height;
int tex_width;
unsigned char* tex_data = nullptr;

glm::mat4 g_mvp;

float imageAspect;

unsigned int m_texture;

bool enable_kernel_editor = false;
void kernel_editor();
void loadImage(Shader cShader, GLFWwindow* window, TriangleFactory& triangleFactory);
void update_mvp(int width, int height);
void GetImgPixel(stbi_uc* image, size_t width, size_t x, size_t y, stbi_uc* r, stbi_uc* g, stbi_uc* b, stbi_uc* a);
void get_new_vertex_positions(float* out);
void new_grid_kernel(float x_max, float y_max, float size, std::vector<float>& hex_vertices, std::vector<int>& indices, std::vector<Hexagon>& hexagons, TriangleFactory& triangleFactory);
void new_tnona_grid_kernel(float x_max, float y_max, float size, std::vector<float>& tnona_vertices, std::vector<int>& indices, std::vector<TNonagon>& tnonagons, TriangleFactory& triangleFactory);
void create_new_grid(TriangleFactory& triangleFactory, int e, int size);
void load_hex_file(TriangleFactory& triangleFactory);
void get_nona_colors(std::array<float, 27> n_vertices, float x_max, float y_max, float px_x_step, float px_y_step, float* colors);
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
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

    io.ConfigWindowsMoveFromTitleBarOnly = true;
    while (!glfwWindowShouldClose(window))
    {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::Begin("Menu");
            static int e = 0;
            static int size = 2;
            ImGui::TextUnformatted("Select resampling grid style");
            ImGui::RadioButton("Hexagon", &e,0);
            ImGui::RadioButton("Truncated Nonagon", &e, 1);
            ImGui::RadioButton("Pentagon", &e, 2);
            ImGui::InputInt("Resampling size", &size);
            if (ImGui::Button("Load Image")) {
                loadImage(cShader, window, triangleFactory);
            }
            if (ImGui::Button("Resampling")) {
                if (m_texture != 0) {
                    create_new_grid(triangleFactory, e, size);
                    glDeleteTextures(1, &m_texture);
                    m_texture = 0;
                }

                if (tex_data != nullptr) {
                    stbi_image_free(tex_data);
                    tex_data = nullptr;
                }
                tex_width = 0;
                tex_height = 0;
                imageAspect = 1.0f;
            }
            if (ImGui::Button("Save")) {

            }
            if (ImGui::Button("Load")) {
                switch (e)
                {
                case 0:
                    load_hex_file(triangleFactory);
                    break;
                default:
                    break;
                }
            }
            if (ImGui::Button("Create Custom Kernel")) {
                enable_kernel_editor = !enable_kernel_editor;
            }
            ImGui::End();
        }
        if (enable_kernel_editor) {
            kernel_editor();
        }
        processInput(window);
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

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

void kernel_editor() {
    ImGui::Begin("Custom Kernel Editor", nullptr, ImGuiChildFlags_AlwaysAutoResize);
    ImGui::BeginChild("Test", ImVec2(120, 220), true);
    ImVec2 p = ImGui::GetCursorScreenPos();
    
    float max_x = 100.0f;
    float max_y = 200.0f;
    static std::vector<float> in_x_all;
    static std::vector<float> in_y_all;
    static std::vector<int> pairs;
    ImGui::EndChild();
    
    static std::string txt = "";
    static int draw_size = 2;
    static std::vector<float> draw_area_x;
    static std::vector<float> draw_area_y;

    draw_area_x.clear();
    draw_area_y.clear();

    if (ImGui::Button("Subdivide Drawing Area")) {
        draw_size *= 2;
    }
    if (ImGui::Button("Decrease Drawing Area")) {
        if (draw_size >= 2) {
            draw_size /= 2;
        }
    }
    float step_x = max_x / draw_size;
    float step_y = max_y / draw_size;
    std::vector<int> draw_area_pairs;
    draw_area_pairs.clear();

    for (size_t y = 0; y < draw_size + 1; y++)
    {
        for (size_t x = 0; x < draw_size + 1; x++)
        {
            draw_area_x.push_back(x * step_x);
            draw_area_y.push_back(y * step_y);

            if (x > 0) {
                draw_area_pairs.push_back(draw_area_x.size() - 1);
                draw_area_pairs.push_back(draw_area_x.size() - 2);
            }
            if (y > 0) {
                draw_area_pairs.push_back(draw_area_x.size() - draw_size - 2);
                draw_area_pairs.push_back(draw_area_x.size() - 1);
            }
        }
    }
    static bool del = false;
    static bool set_region = false;
    static int cur_region = -1;
    ImGui::Checkbox("Delete Point", &del);
    ImGui::Checkbox("Create Region", &set_region);
    ImGui::InputInt("Select Region", &cur_region);
    static std::vector<ImVec2> region_points;
    static std::vector<std::vector<ImVec2>> regions;
    if (!set_region) {
        region_points.clear();
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 pos = ImGui::GetMousePos();
        if (del) {
            for (size_t i = 0; i < in_x_all.size(); i++)
            {
                if (pos.x <  p.x + in_x_all[i] + 5 && pos.x > p.x + in_x_all[i] - 5 &&
                    pos.y < p.y + in_y_all[i] + 5 && pos.y > p.y + in_y_all[i] - 5) {
                    in_x_all.erase(in_x_all.begin() + i);
                    in_y_all.erase(in_y_all.begin() + i);
                    break;
                }
            }
        }
        else if (set_region) {
            for (size_t i = 0; i < in_x_all.size(); i++)
            {
                if (pos.x <  p.x + in_x_all[i] + 5 && pos.x > p.x + in_x_all[i] - 5 &&
                    pos.y < p.y + in_y_all[i] + 5 && pos.y > p.y + in_y_all[i] - 5) {
                    region_points.push_back(ImVec2(p.x + in_x_all[i], p.y + in_y_all[i]));
                    break;
                }
            }
        }
        else {
            for (size_t i = 0; i < draw_area_x.size(); i++)
            {
                if (pos.x <  p.x + draw_area_x[i] + 5 && pos.x > p.x + draw_area_x[i] - 5 &&
                    pos.y < p.y + draw_area_y[i] + 5 && pos.y > p.y + draw_area_y[i] - 5) {
                    in_x_all.push_back(draw_area_x[i]);
                    in_y_all.push_back(draw_area_y[i]);
                    break;
                }
            }
        }
        
    }
    if (ImGui::Button("Submit Region")) {
        regions.push_back(std::move(region_points));
        region_points.clear();
    }

    for (size_t i = 0; i < regions.size(); i++)
    {
        if (i == cur_region) {
            ImGui::GetWindowDrawList()->AddConcavePolyFilled(regions[i].data(), regions[i].size(), IM_COL32(255, 255, 0, 100));
        }
        else {
            ImGui::GetWindowDrawList()->AddConcavePolyFilled(regions[i].data(), regions[i].size(), IM_COL32(255, 0, 0, 100));
        }
    }

    for (size_t i = 0; i < draw_area_pairs.size(); i += 2)
    {
        //ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + draw_area_x[draw_area_pairs[i]], p.y + draw_area_y[draw_area_pairs[i]]), ImVec2(p.x + draw_area_x[draw_area_pairs[i + 1]], p.y + draw_area_y[draw_area_pairs[i + 1]]), IM_COL32(255, 0, 0, 255), 3.0f);
        ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + draw_area_x[draw_area_pairs[i]], p.y + draw_area_y[draw_area_pairs[i]]), ImVec2(p.x + draw_area_x[draw_area_pairs[i + 1]], p.y + draw_area_y[draw_area_pairs[i + 1]]), IM_COL32(255, 255, 255, 100), 1.0f);
    }
    for (size_t i = 0; i < in_x_all.size(); i++)
    {
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + in_x_all[i], p.y + in_y_all[i]), 3.0f, IM_COL32(255, 0, 0, 255));
        for (size_t j = 0; j < region_points.size(); j++)
        {
            if (p.x + in_x_all[i] == region_points[j].x && p.y + in_y_all[i] == region_points[j].y) {
                ImGui::GetWindowDrawList()->AddCircleFilled(region_points[j], 3.0f, IM_COL32(255, 255, 0, 255));
            }
        }
    }
    ImGui::End();
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

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
void get_tri_colors(std::array<float, 9> t_vertices, float x_max, float y_max, float px_x_step, float px_y_step, float* colors) {
    std::vector<int> x;
    std::vector<int> y;
    for (size_t v = 0; v < 9; v += 3)
    {
        x.push_back((int)((t_vertices[v] + x_max) / px_x_step));
        y.push_back((int)((t_vertices[v + 1] + y_max) / px_y_step));
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

    r = (r / count) / 255;
    g = (g / count) / 255;
    b = (b / count) / 255;
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
void get_nona_colors(std::array<float, 27> n_vertices, float x_max, float y_max, float px_x_step, float px_y_step, float* colors) {
    std::vector<int> x;
    std::vector<int> y;
    for (size_t v = 0; v < 27; v += 3)
    {
        x.push_back((int)((n_vertices[v] + x_max) / px_x_step));
        y.push_back((int)((n_vertices[v + 1] + y_max) / px_y_step));
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

    r = (r / count) / 255;
    g = (g / count) / 255;
    b = (b / count) / 255;
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
void new_tnona_grid_kernel(float x_max, float y_max, float size, std::vector<float>& tnona_vertices, std::vector<int>& indices, std::vector<TNonagon>& tnonagons, TriangleFactory& triangleFactory) {

    float px_x_step = (x_max * 2) / (tex_width - 1);
    float px_y_step = (y_max * 2) / (tex_height - 1);

    const int VERTS_PER_CELL = 14;
    const int FLOATS_PER_VERT = 3;
    const int FLOATS_PER_CELL = VERTS_PER_CELL * FLOATS_PER_VERT;
    const int INDICES_PER_CELL = 42;

    float x_step = (2.0f * x_max) / (size * 2);
    float y_step = (2.0f * y_max) / size;

    float cx;
    float cy;
    float colors[9];
    std::array<float, FLOATS_PER_CELL> v;
    std::array<float, FLOATS_PER_CELL> prev_v;
    std::array<float, 27> n_v;
    std::array<float, 9> t_v;
    std::array<int, 21> n_i;
    std::array<int, 3> t_i;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size * 2; x++) {

            cx = -x_max + x * x_step;
            cy = y_max - y * y_step;

            v = {
                cx,             cy,               0.0f,
                cx + x_step,    cy,               0.0f,
                cx,             cy - y_step * 0.2f,               0.0f,
                cx + x_step,    cy - y_step * 0.2f,               0.0f,
                cx,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),                   0.0f,
                cx + x_step * 0.1f,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                cx + x_step * 0.9f,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                cx + x_step,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),           0.0f,
                cx + x_step * 0.5f,    cy - y_step * 0.5f,        0.0f,
                cx + x_step * 0.5f,    cy - y_step * 0.7f,        0.0f,
                cx + x_step * 0.4f,    cy - y_step * (0.7f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                cx + x_step * 0.6f,    cy - y_step * (0.7f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                cx,    cy - y_step,            0.0f,
                cx + x_step,    cy - y_step,   0.0f
            };
            if (y > 0) {
                cx = -x_max + x * x_step;
                cy = y_max - (y - 1) * y_step;

                prev_v = {
                    cx,             cy,               0.0f,
                    cx + x_step,    cy,               0.0f,
                    cx,             cy - y_step * 0.2f,               0.0f,
                    cx + x_step,    cy - y_step * 0.2f,               0.0f,
                    cx,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),                   0.0f,
                    cx + x_step * 0.1f,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                    cx + x_step * 0.9f,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                    cx + x_step,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),           0.0f,
                    cx + x_step * 0.5f,    cy - y_step * 0.5f,        0.0f,
                    cx + x_step * 0.5f,    cy - y_step * 0.7f,        0.0f,
                    cx + x_step * 0.4f,    cy - y_step * (0.7f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                    cx + x_step * 0.6f,    cy - y_step * (0.7f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                    cx,    cy - y_step,            0.0f,
                    cx + x_step,    cy - y_step,   0.0f
                };

                n_v = {
                    prev_v[30], prev_v[31], prev_v[32],
                    prev_v[33], prev_v[34], prev_v[35],
                    prev_v[36], prev_v[37], prev_v[38],
                    prev_v[39], prev_v[40], prev_v[41],

                    v[6],  v[7], v[8],
                    v[9],  v[10], v[11],
                    v[15], v[16], v[17],
                    v[18], v[19], v[20],
                    v[24], v[25], v[26]
                };
                n_i = {
                    0, 1, 2,
                    1, 2, 3,
                    2, 3, 4,
                    3, 4, 5,
                    4, 5, 6,
                    5, 6, 7,
                    6, 7, 8
                };
                
                if (y == 1) {
                    t_v = {
                        v[27], v[28], v[29],
                        v[30], v[31], v[32],
                        v[33], v[34], v[35],
                    };
                    t_i = {
                        0, 1, 2
                    };
                    get_tri_colors(t_v, x_max, y_max, px_x_step, px_y_step, colors);
                    float t[9];
                    for (size_t k = 0; k < 3; k++)
                    {
                        int vi = t_i[k];
                        t[k * 3 + 0] = t_v[vi * 3 + 0];
                        t[k * 3 + 1] = t_v[vi * 3 + 1];
                        t[k * 3 + 2] = t_v[vi * 3 + 2];
                    }
                    Triangle t1(t, colors);
                    triangleFactory.addTriangle(t1);
                    t_v = {
                        prev_v[27], prev_v[28], prev_v[29],
                        prev_v[30], prev_v[31], prev_v[32],
                        prev_v[33], prev_v[34], prev_v[35],
                    };
                    t_i = {
                        0, 1, 2
                    };
                    get_tri_colors(t_v, x_max, y_max, px_x_step, px_y_step, colors);
                    for (size_t k = 0; k < 3; k++)
                    {
                        int vi = t_i[k];
                        t[k * 3 + 0] = t_v[vi * 3 + 0];
                        t[k * 3 + 1] = t_v[vi * 3 + 1];
                        t[k * 3 + 2] = t_v[vi * 3 + 2];
                    }
                    Triangle t2(t, colors);
                    triangleFactory.addTriangle(t2);
                }
                else {
                    t_v = {
                        v[27], v[28], v[29],
                        v[30], v[31], v[32],
                        v[33], v[34], v[35],
                    };
                    t_i = {
                        0, 1, 2
                    };
                    get_tri_colors(t_v, x_max, y_max, px_x_step, px_y_step, colors);
                    float t[9];
                    for (size_t k = 0; k < 3; k++)
                    {
                        int vi = t_i[k];
                        t[k * 3 + 0] = t_v[vi * 3 + 0];
                        t[k * 3 + 1] = t_v[vi * 3 + 1];
                        t[k * 3 + 2] = t_v[vi * 3 + 2];
                    }
                    Triangle t1(t, colors);
                    triangleFactory.addTriangle(t1);
                }
                get_nona_colors(n_v, x_max, y_max, px_x_step, px_y_step, colors);
                /*float colors[9] = {
                    0.1f, 0.1f * x, 0.1f *y,
                    0.1f, 0.1f * x, 0.1f * y,
                    0.1f, 0.1f * x, 0.1f * y,
                };*/
                float t[9];
                for (size_t j = 0; j < 7; j++)
                {

                    for (size_t k = 0; k < 3; k++)
                    {
                        int vi = n_i[j * 3 + k];
                        t[k * 3 + 0] = n_v[vi * 3 + 0];
                        t[k * 3 + 1] = n_v[vi * 3 + 1];
                        t[k * 3 + 2] = n_v[vi * 3 + 2];
                    }
                    Triangle t1(t, colors);
                    triangleFactory.addTriangle(t1);
                }
                //Hexagon h(x, y * 2 - 1, colors);
                //hexagons.push_back(h);
                //create_hex_triangle(h_i, h_v, colors, triangleFactory);
            }
            if (x > 0) {
                cx = -x_max + (x - 1) * x_step;
                cy = y_max - y * y_step;

                prev_v = {
                    cx,             cy,               0.0f,
                    cx + x_step,    cy,               0.0f,
                    cx,             cy - y_step * 0.2f,               0.0f,
                    cx + x_step,    cy - y_step * 0.2f,               0.0f,
                    cx,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),                   0.0f,
                    cx + x_step * 0.1f,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                    cx + x_step * 0.9f,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                    cx + x_step,    cy - y_step * (0.2f + std::sqrtf(0.75f * 0.04f)),           0.0f,
                    cx + x_step * 0.5f,    cy - y_step * 0.5f,        0.0f,
                    cx + x_step * 0.5f,    cy - y_step * 0.7f,        0.0f,
                    cx + x_step * 0.4f,    cy - y_step * (0.7f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                    cx + x_step * 0.6f,    cy - y_step * (0.7f + std::sqrtf(0.75f * 0.04f)),    0.0f,
                    cx,    cy - y_step,            0.0f,
                    cx + x_step,    cy - y_step,   0.0f
                };

                n_v = {
                    prev_v[18], prev_v[19], prev_v[20],
                    v[15], v[16], v[17],
                    prev_v[24], prev_v[25], prev_v[26],
                    v[24], v[25], v[26],
                    prev_v[27], prev_v[28], prev_v[29],
                    v[27], v[28], v[29],
                    prev_v[33], prev_v[34], prev_v[35],
                    v[30], v[31], v[32],
                    v[36], v[37], v[38],
                };
                n_i = {
                    0, 1, 2,
                    1, 2, 3,
                    2, 3, 4,
                    3, 4, 5,
                    4, 5, 6,
                    5, 6, 7,
                    6, 7, 8
                };
                if (y > 0) {
                    t_v = {
                        prev_v[18], prev_v[19], prev_v[20],
                        v[6], v[7], v[8],
                        v[15], v[16], v[17]
                    };
                    t_i = {
                        0, 2, 1
                    };
                    get_tri_colors(t_v, x_max, y_max, px_x_step, px_y_step, colors);
                    float t[9];
                    for (size_t k = 0; k < 3; k++)
                    {
                        int vi = t_i[k];
                        t[k * 3 + 0] = t_v[vi * 3 + 0];
                        t[k * 3 + 1] = t_v[vi * 3 + 1];
                        t[k * 3 + 2] = t_v[vi * 3 + 2];
                    }
                    Triangle t1(t, colors);
                    triangleFactory.addTriangle(t1);
                }

                get_nona_colors(n_v, x_max, y_max, px_x_step, px_y_step, colors);
                /*float colors[9] = {
                    0.1f, 0.1f * x, 0.1f * y,
                    0.1f, 0.1f * x, 0.1f * y,
                    0.1f, 0.1f * x, 0.1f * y,
                };*/
                float t[9];
                for (size_t j = 0; j < 7; j++)
                {

                    for (size_t k = 0; k < 3; k++)
                    {
                        int vi = n_i[j * 3 + k];
                        t[k * 3 + 0] = n_v[vi * 3 + 0];
                        t[k * 3 + 1] = n_v[vi * 3 + 1];
                        t[k * 3 + 2] = n_v[vi * 3 + 2];
                    }
                    Triangle t1(t, colors);
                    triangleFactory.addTriangle(t1);
                }
                //Hexagon h(x, y * 2, colors);
                //hexagons.push_back(h);
                //create_hex_triangle(h_i, h_v, colors, triangleFactory);
            }
        }
    }
    triangleFactory.finalizeBuffer();
    /*std::string final = "";
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
    hex_file.close();*/
}
void create_new_grid(TriangleFactory& triangleFactory, int e, int size) {
    float corner_vertices[12];
    get_new_vertex_positions(corner_vertices);
    switch (e)
    {
    case 0: 
    {
        const int hex_arr_size = size * (size * 2 * 24);
        std::vector<float> hex_vertices(hex_arr_size);
        std::vector<int> indices(hex_arr_size);
        std::vector<Hexagon> hexagons;
        new_grid_kernel(corner_vertices[0], corner_vertices[1], size, hex_vertices, indices, hexagons, triangleFactory);

        break;
    }
    case 1: 
    {
        const int tnona_arr_size = size * (size * 2 * 42);
        std::vector<float> tnona_vertices(tnona_arr_size);
        std::vector<int> indices(tnona_arr_size);
        std::vector<TNonagon> tnonagons;
        new_tnona_grid_kernel(corner_vertices[0], corner_vertices[1], size, tnona_vertices, indices, tnonagons, triangleFactory);
        break;
    }
    }
    
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