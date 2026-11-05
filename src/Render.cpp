#include "Render.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

using namespace UI;

void Render::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Render::draw() {
    Recti graphPanel = rect.subRect(0.0, 0.7, 0.0, 1.0).inset(20, 20, 20, 20);
    drawRect(graphPanel, Vector3(0.13, 0.135, 0.16));
    drawRect(graphPanel.subRect(0.0, 1.0, 0.0, 0.5).inset(20, 20, 20, 10), Vector3(0.09, 0.095, 0.12));
    drawRect(graphPanel.subRect(0.0, 1.0, 0.5, 1.0).inset(20, 20, 10, 20), Vector3(0.09, 0.095, 0.12));
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

Render::Render(int w, int h, const char* title, GLFWmonitor* monitor, GLFWwindow* share) {
    window = glfwCreateWindow(w, h, title, monitor, share);
    rect.position = Vector2i(0, 0);
    if (!window) throw std::runtime_error("Failed to create window.");
    glfwSetKeyCallback(window, keyCallback);
    glfwMakeContextCurrent(window);
    while (!glfwWindowShouldClose(window)) {
        // Do one frame of sorting, if applicable

        glfwGetFramebufferSize(window, &rect.size.x, &rect.size.y);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, rect.size.x, rect.size.y, 0, -1, 1);
 
        glViewport(0, 0, rect.size.x, rect.size.y);
        glClearColor(0.09, 0.095, 0.12, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}