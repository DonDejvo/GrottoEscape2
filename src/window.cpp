#include "window.h"
#include <assert.h>

Window::Window(uint32_t w, uint32_t h)
    : mWidth(w), mHeight(h)
{
}

void Window::Init() {
    if(!glfwInit()) return;

    assert(mWidth != 0);
    assert(mHeight != 0);

#if defined(BACKEND_GLES)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif
    mWindow = glfwCreateWindow(mWidth, mHeight, "Grotto Escape 2", nullptr, nullptr);

    assert(mWindow != nullptr);
}

bool Window::IsRunning() {
    return glfwWindowShouldClose(mWindow) == 0;
}

void Window::Destroy()
{
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}
