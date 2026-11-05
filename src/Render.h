#pragma once

#include <GLFW/glfw3.h>

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
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void draw();
    void drawRect(const Recti& r, const Vector3& color);
public:
    Render(int w, int h, const char* title, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr);
};