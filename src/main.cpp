#include "Render.h"

#include <GLFW/glfw3.h>

int main(int argc, char* argv[]) {
    if (!glfwInit()) return -1;
    Render render(1280, 720, "Supermarket Sort");   // Starts the main process loop
    return 0;
}