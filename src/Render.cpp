#define STB_TRUETYPE_IMPLEMENTATION

#include "Render.h"
#include "Dataset.h"
#include "Sort.h"
#include "../fonts/stb_truetype.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <map>
#include <iostream>

using namespace UI;

Render* Render::__instance;

void Render::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Render::__instance->processInput(key, scancode, action, mods);
}

void Render::mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    Render::__instance->processInput(button, 0, action, mods);
}

// PERFORM COMMAND (ie button press)
// 0 - Merge sort by Customer Age
// 1 - Merge sort by Loyalty Points Earned
// 2 - Merge sort by Unit Price
// 3 - Merge sort by Popularity Score
// 4 - Quick sort by Customer Age
// 5 - Quick sort by Loyalty Points Earned
// 6 - Quick sort by Unit Price
// 7 - Quick sort by Popularity Score
void Render::command(int cmd) {
    if (cmd == 0 || cmd == 4) sort_col = 8;
    if (cmd == 1 || cmd == 5) sort_col = 26;
    if (cmd == 2 || cmd == 6) sort_col = 111;
    if (cmd == 3 || cmd == 7) sort_col = 133;
    if (cmd >= 0 && cmd <= 3) {
        sorting = true;
        MergeSort(sort_col);
        sorting = false;
    }
    if (cmd >= 4 && cmd <= 7) {
        sorting = true;
        QuickSort(sort_col);
        sorting = false;
    } 
}

bool Render::draw(const std::unordered_map<int,int>* const m_r_map, const std::unordered_map<int,int>* const q_r_map, const Dataset* const data) {
    if (glfwWindowShouldClose(window)) {
        glfwTerminate();
        return false;
    }

    glfwGetFramebufferSize(window, &rect.size.x, &rect.size.y);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, rect.size.x, rect.size.y, 0, -1, 1);

    glViewport(0, 0, rect.size.x, rect.size.y);
    glClearColor(0.09, 0.095, 0.12, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    Recti headerPanel = rect.subRect(0.0, 0.15, 0.0, 1.0).inset(20, 0, 20, 20);
    drawString(headerPanel.subRect(0.05, 0.5, 0.0, 1.0), "Supermarket Sort");
    drawString(headerPanel.subRect(0.65, 1.0, 0.0, 0.45), "Merge Sort");
    drawString(headerPanel.subRect(0.65, 1.0, 0.55, 1.0), "Quick Sort");

    Recti graphPanel = rect.subRect(0.15, 0.8, 0.0, 1.0).inset(20, 20, 20, 20);
    drawRect(graphPanel, Vector3(0.13, 0.135, 0.16));
    drawRect(graphPanel.subRect(0.0, 1.0, 0.0, 0.5).inset(20, 20, 20, 10), Vector3(0.09, 0.095, 0.12));
    drawRect(graphPanel.subRect(0.0, 1.0, 0.5, 1.0).inset(20, 20, 10, 20), Vector3(0.09, 0.095, 0.12));

    Recti buttonsPanel = rect.subRect(0.8, 1.0, 0.0, 1.0).inset(0, 20, 20, 20);
    drawButton(buttonsPanel.subRect(0.0, 0.5, 0.0, 0.25).inset(0, 10, 0, 10), "Age", 0);
    drawButton(buttonsPanel.subRect(0.0, 0.5, 0.25, 0.5).inset(0, 10, 10, 10), "Loyalty Points", 1);
    drawButton(buttonsPanel.subRect(0.5, 1.0, 0.0, 0.25).inset(10, 0, 0, 10), "Unit Price", 2);
    drawButton(buttonsPanel.subRect(0.5, 1.0, 0.25, 0.5).inset(10, 0, 10, 10), "Popularity Score", 3);
    drawButton(buttonsPanel.subRect(0.0, 0.5, 0.5, 0.75).inset(0, 10, 10, 10), "Age", 4);
    drawButton(buttonsPanel.subRect(0.0, 0.5, 0.75, 1.0).inset(0, 10, 10, 0), "Loyalty Points", 5);
    drawButton(buttonsPanel.subRect(0.5, 1.0, 0.5, 0.75).inset(10, 0, 10, 10), "Unit Price", 6);
    drawButton(buttonsPanel.subRect(0.5, 1.0, 0.75, 1.0).inset(10, 0, 10, 0), "Popularity Score", 7);

    if (data) {
        drawBarGraph(graphPanel.subRect(0.0, 1.0, 0.0, 0.5).inset(20, 20, 20, 10), m_r_map, data);    // Draw merge sort bar graph
        drawBarGraph(graphPanel.subRect(0.0, 1.0, 0.5, 1.0).inset(20, 20, 10, 20), q_r_map, data);    // Draw quick sort bar graph
    }

    glfwSwapBuffers(window);
    glfwPollEvents();

    return true;
}

void Render::drawButton(Recti r, const std::string& label, int cmd, const Vector3& color) {
    drawRect(r, color);
    drawString(r.subRect(0.3, 0.7, 0.0, 1.0), label);
    commands[cmd] = r;
}

void Render::drawRect(const Recti& r, const Vector3& color) {
    glColor3f(color.x, color.y, color.z);
    glBegin(GL_QUADS);
    glVertex2f(r.position.x, r.position.y);
    glVertex2f(r.position.x + r.size.x, r.position.y);
    glVertex2f(r.position.x + r.size.x, r.position.y + r.size.y);
    glVertex2f(r.position.x, r.position.y + r.size.y);
    glEnd();
}

void Render::drawString(const Recti& r, const std::string& text, const Vector3& color, const std::string& typeface)
{
    int stringWidth = (int)((text.size() + 1) * r.size.y * 0.6);
    int x = r.position.x + r.size.x / 2 - stringWidth / 2;
    int y = r.position.y + r.size.y;
    float fontHeight = (float)r.size.y * 1.5;

    struct Glyph {
        float x0, y0, x1, y1;
        float xoff, yoff, advance;
    };
    static GLuint texture = 0;
    static Glyph glyphs[128];
    std::ifstream file("fonts/" + typeface + ".ttf", std::ios::binary);
    if (!file) throw std::runtime_error("Failed to load font \"" + typeface + "\".");

    std::vector<unsigned char> font((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    constexpr int atlasWidth = 1024;
    constexpr int atlasHeight = 512;

    std::vector<unsigned char> bitmap(atlasWidth * atlasHeight, 0);
    stbtt_bakedchar baked[96];
    int result = stbtt_BakeFontBitmap(font.data(), 0, fontHeight, bitmap.data(), atlasWidth, atlasHeight, 32, 96, baked);
    if (result <= 0) throw std::runtime_error("Font atlas too small.");

    for (int i = 0; i < 96; ++i)
    {
        const auto& b = baked[i];

        glyphs[i + 32] = {
            (float)b.x0 / atlasWidth,
            (float)b.y0 / atlasHeight,
            (float)b.x1 / atlasWidth,
            (float)b.y1 / atlasHeight,
            b.xoff,
            b.yoff,
            b.xadvance
        };
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_ALPHA,
        atlasWidth, atlasHeight,
        0,
        GL_ALPHA,
        GL_UNSIGNED_BYTE,
        bitmap.data()
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(color.x, color.y, color.z, 1);
    glBegin(GL_QUADS);
    for (unsigned char c : text)
    {
        if (c < 32 || c > 127)
            continue;

        const Glyph& g = glyphs[c];

        float x0 = x + g.xoff;
        float y0 = y + g.yoff;
        float x1 = x0 + (g.x1 - g.x0) * 1024;
        float y1 = y0 + (g.y1 - g.y0) * 512;

        glTexCoord2f(g.x0, g.y0);
        glVertex2f(x0, y0);

        glTexCoord2f(g.x1, g.y0);
        glVertex2f(x1, y0);

        glTexCoord2f(g.x1, g.y1);
        glVertex2f(x1, y1);

        glTexCoord2f(g.x0, g.y1);
        glVertex2f(x0, y1);

        x += g.advance;
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void Render::drawBarGraph(const Recti& r, const std::unordered_map<int,int>* const r_map, const Dataset* const data) {
    if (!r_map || !data || sort_col < 0) 
        return;
    double max_value = 0.0;
    std::map<int, double> bars;
    for (std::pair<int, int> r : *r_map) {
        double num = data->number(sort_col, r.first * 1000 - 1);
        bars[r.second] = num;
        max_value = std::max(max_value, num);
    }
    int size = bars.size();
    int i = 0;
    for (std::pair<int, double> bar : bars) {
        drawRect(r.subRect(1.0 - bar.second / max_value, 1.0, (float)i / size, (float)(i + 1) / size), Vector3(0.85, 0.3, 0.35));
        i++;
    }
}

void Render::processInput(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
    if (key == GLFW_MOUSE_BUTTON_1) {
        if (sorting) 
            return;
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        Vector2i mousePos = Vector2i((int)x, (int)y);
        if (!rect.hasPoint(mousePos)) return;
        for (std::pair<int, Recti> cmd : commands) {
            if (cmd.second.hasPoint(mousePos)) {
                command(cmd.first);
                return;
            }
        }
        return;
    }
}

Render::Render(int w, int h, const char* title, GLFWmonitor* monitor, GLFWwindow* share) {
    __instance = this;
    window = glfwCreateWindow(w, h, title, monitor, share);
    rect.position = Vector2i(0, 0);
    sort_col = -1;
    sorting = false;

    if (!window) throw std::runtime_error("Failed to create window.");
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseCallback);
    glfwMakeContextCurrent(window);
}