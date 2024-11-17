#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glh/glh.h"

#include "OpenGLApplication.h"

#include <iostream>
#include <chrono>
#include <array>
#include <numeric>

OpenGLApplication::ApplicationConfig appConfig{};

constexpr int MOVE_FACTOR = 100;
constexpr int FRAMETIME_ROLLING_AVG_SAMPLE_SIZE = 100;

float size = 20.0f;

class KeyState {
public:
    KeyState(int inKey) : key(inKey) {}
    void update(int inKey, int inAction) {
        if (key == inKey) {
            if (inAction == GLFW_PRESS) {
                active = true;
            }
            else if (inAction == GLFW_RELEASE) {
                active = false;
            }
        }
    }
    bool isActive() { return active; }

private:
    int key;
    bool active;
};

void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {

    static KeyState left(GLFW_KEY_LEFT);
    static KeyState right(GLFW_KEY_RIGHT);
    static KeyState up(GLFW_KEY_UP);
    static KeyState down(GLFW_KEY_DOWN);

    left.update(key, action);
    right.update(key, action);
    up.update(key, action);
    down.update(key, action);

    bool ctrl = mods & GLFW_MOD_CONTROL;
    bool shift = mods & GLFW_MOD_SHIFT;
    bool alt = mods & GLFW_MOD_ALT;
    bool noMod = !ctrl && !shift && !alt;


    if (key == GLFW_KEY_ESCAPE && (action == GLFW_PRESS)) {
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    }

    if (left.isActive() && shift) {
        size -= 2.0f;
        if (size < 1.0f) size = 2.0f;
    }

    if (right.isActive() && shift) {
        size += 2.0f;
    }

    if (left.isActive() && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos - MOVE_FACTOR, yPos);
    }

    if (right.isActive() && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos + MOVE_FACTOR, yPos);
    }

    if (up.isActive() && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos, yPos - MOVE_FACTOR);
    }

    if (down.isActive() && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos, yPos + MOVE_FACTOR);
    }
}

std::string doubleToString(double val, size_t precision) {
    std::string str = std::to_string(val);
    int dec = str.find('.');
    return str.substr(0, dec + precision + 1);
}

void draw(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    D3DInteropTexture2D::initDirect3D();
    glhInitFont(appConfig.windowInitWidth, appConfig.windowInitHeight);

    D3DInteropTexture2D tex(100, 100, false);
    tex.interopLock(); // Leave it locked forever, until destruction

    Framebuffer framebuf;
    framebuf.bind();

    framebuf.setColorAttachment2D(GL_TEXTURE_2D, tex.handle());

    tex.interopUnlock();

    if (!framebuf.isComplete()) {
        std::cout << "ERROR draw() Framebuffer is incomplete! Rendering will fail." << std::endl;
    }

    glEnable(GL_TEXTURE_2D);

    size_t writeIndex = 0;
    std::array<double, FRAMETIME_ROLLING_AVG_SAMPLE_SIZE> frametimeMsTimings;

    while (!glfwWindowShouldClose(window)) {
        auto begin = std::chrono::high_resolution_clock::now();

        tex.interopLock();
        framebuf.bind();

        float sinVal = sinf((float)glfwGetTime()) * 0.5f + 0.5f;
        glhClearColor(sinVal, 0.0f, sinVal, 1.0f);
        glhClear(GL_COLOR_BUFFER_BIT);

        framebuf.unbind(); // Binds default framebuffer

        glhClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glhClear(GL_COLOR_BUFFER_BIT);

        tex.bind();

        glhBegin(GL_TRIANGLES);

        glhTexCoord2f(0.0f, 0.0f);
        glhVertex2f(-1.0f, -1.0f);

        glhTexCoord2f(0.0f, 1.0f);
        glhVertex2f(0.0f, 1.0f);

        glhTexCoord2f(1.0f, 0.0f);
        glhVertex2f(1.0f, 0.0f);

        glhEnd();

        tex.interopUnlock();

        std::string timeStr = doubleToString(glfwGetTime(), 2) + "s";
        FontRect timeRect = glhDrawText(timeStr, 0, 0, size);

        double total = std::accumulate(std::begin(frametimeMsTimings), std::end(frametimeMsTimings), 0);
        double frametimeMsAvg = total / frametimeMsTimings.size();
        std::string frametimeStr = doubleToString(frametimeMsAvg, 2) + " frametime (ms)";
        glhDrawText(frametimeStr, 0, timeRect.height, size);
        
        std::string moveText = "Ctrl+Arrow Keys to move window";
        FontRect moveTextRect = glhGetTextSize(moveText, size);
        glhDrawText(moveText, appConfig.windowInitWidth - moveTextRect.width, 0, size);

        std::string scaleText = "Shift+Arrow Keys to scale text";
        FontRect scaleTextRect = glhGetTextSize(scaleText, size);
        glhDrawText(scaleText, appConfig.windowInitWidth - scaleTextRect.width, moveTextRect.height, size);

        glhErrorCheck("End of Render");

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = end - begin;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        frametimeMsTimings[writeIndex] = ms;
        writeIndex++;
        writeIndex %= FRAMETIME_ROLLING_AVG_SAMPLE_SIZE;

        glfwSwapBuffers(window);
    }
    glhFreeFont();
    D3DInteropTexture2D::shutdownDirect3D();
}

int main(int argc, char argv[])
{
    appConfig.windowName = "Producer";
    appConfig.windowInitWidth = 1000;
    appConfig.windowInitHeight = 1000;
    appConfig.windowPosX = 200;
    appConfig.windowPosY = 100;
    appConfig.windowBorderless = false;
    appConfig.windowResizeEnable = false;
    appConfig.windowDarkmode = true;
    appConfig.windowRounded = true;
    appConfig.glVersionMajor = 4;
    appConfig.glVersionMinor = 6;
    appConfig.customDrawFunc = &draw;
    appConfig.customKeyCallback = &keyCallback;

    try {
        OpenGLApplication application(appConfig);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}