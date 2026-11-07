#pragma once

#include "Dataset.h"

#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>

namespace UI {
    struct Vector2 {
        float x; 
        float y; 
        Vector2() {}
        Vector2(float _x, float _y) : x(_x), y(_y) {}
    };
    struct Vector3 {
        float x;
        float y;
        float z;
        Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    };
    struct Vector2i {
        int x;
        int y;
        Vector2i() {}
        Vector2i(int _x, int _y) : x(_x), y(_y) {}
        Vector2i operator+(const Vector2i& v) const { return Vector2i(x + v.x, y + v.y); }
        Vector2i operator-(const Vector2i& v) const { return Vector2i(x - v.x, y - v.y); }
        Vector2i operator*(const Vector2& v) const { return Vector2i((int)(x * v.x), (int)(y * v.y)); }
    };
    struct Vector3i {
        int x;
        int y;
        int z;
        Vector3i(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
    };
    struct Recti {
        Vector2i position; 
        Vector2i size;
        Recti() {}
        Recti(Vector2i _position, Vector2i _size) : position(_position), size(_size) {}
        Recti subRect(float u, float d, float l, float r) const {
            return Recti(position + size * Vector2(l, u), size * Vector2(r - l, d - u));
        }
        Recti inset(int uOffset, int dOffset, int lOffset, int rOffset) const { 
            return Recti(position + Vector2i(lOffset, uOffset), size - Vector2i(lOffset + rOffset, uOffset + dOffset));
        }
        bool hasPoint(Vector2i v) {
            return v.x >= position.x && v.x <= position.x + size.x &&
                v.y >= position.y && v.y <= position.y + size.y;
        }
    };
    const Vector2 UL = Vector2(0, 0);
    const Vector2 UM = Vector2(0.5, 0);
    const Vector2 UR = Vector2(1, 0);
    const Vector2 ML = Vector2(0, 0.5);
    const Vector2 MM = Vector2(0.5, 0.5);
    const Vector2 MR = Vector2(1, 0.5);
    const Vector2 BL = Vector2(0, 1);
    const Vector2 BM = Vector2(0.5, 1);
    const Vector2 BR = Vector2(1, 1);
}   // namespace

using namespace UI;

class Render {
    GLFWwindow* window;
    Recti rect;
    int sort_col;
    bool sorting;
    std::unordered_map<int, Recti> commands;
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* window, int button, int action, int mods);
    void command(int cmd);
    void drawButton(Recti r, const std::string& label, int cmd, const Vector3& color = Vector3(0.13, 0.135, 0.16));
    void drawRect(const Recti& r, const Vector3& color);
    void drawString(const Recti& r, const std::string& text, const Vector3& color = Vector3(1.0, 1.0, 1.0), const std::string& typeface = "JetBrainsMono-Regular");
    void drawBarGraph(const Recti& r, const std::unordered_map<int,int>* const r_map, const Dataset* const data);
public:
    static Render* __instance;
    void processInput(int key, int scancode, int action, int mods);
    bool draw(const std::unordered_map<int,int>* const m_r_map, const std::unordered_map<int,int>* const q_r_map, const Dataset* const data);
    Render(int w, int h, const char* title, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr);
};