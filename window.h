#pragma once

#include <GLFW/glfw3.h>

class Window {
private:
    uint32_t mWidth;
    uint32_t mHeight;
    GLFWwindow* mWindow;
public:
    Window(uint32_t w, uint32_t h);
    void Init();
    bool IsRunning();
    void Destroy();

    inline GLFWwindow* getGLFWWindow() {
        return mWindow;
    };

    inline uint32_t getWidth() const {
        return mWidth;
    }

    inline uint32_t getHeight() const {
        return mHeight;
    }
};