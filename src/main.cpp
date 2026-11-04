#include <iostream>
#include <GLFW/glfw3.h>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main(int argc, char* argv[]) {
    if (!glfwInit()) return -1;
    GLFWwindow* win = glfwCreateWindow(640, 480, "Supermarket Sort", NULL, NULL);
    if (!win) return -1;
    glfwSetKeyCallback(win, key_callback);
    while (!glfwWindowShouldClose(win)) {
        int width, height;
        glfwGetFramebufferSize(win, &width, &height);
 
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
 
        glfwSwapBuffers(win);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}